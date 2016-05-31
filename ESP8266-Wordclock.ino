#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ESP8266mDNS.h>
#include <PubSubClient.h>
#include <NTPClient.h>
#include <NeoPixelBus.h>
#include "fonts/6x8_vertikal_MSB_1.h"

bool debug = true;
#define SerialDebug(text)   Serial.print(text);
#define SerialDebugln(text) Serial.println(text);

char MyIp[16];
char MyHostname[16];
char MyRoom[32]="";

WiFiClient espMqttClient;
PubSubClient mqttClient(espMqttClient);

WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "de.pool.ntp.org", 60*60, 60*60*1000);

#define panelWidth  11
#define panelHeight 10
#define pixelCount  (panelWidth*panelHeight+4)
#define pixelPin    2          // should be ignored because of NeoEsp8266Dma800KbpsMethod
NeoPixelBus<NeoGrbFeature, NeoEsp8266Uart800KbpsMethod> strip(pixelCount, pixelPin);
NeoTopology <ColumnMajorAlternatingLayout> topo(panelWidth, panelHeight);
#define MAP(X,Y)  (topo.Map((X)-1, (Y)-1)+2)
RgbColor black = RgbColor(0);
RgbColor color = RgbColor(255,255,196);
uint8_t mode=1;

String text("");
#define FontWidth 6
#define MaxTextLen  255
byte Text[MaxTextLen];
int TextPos=0;
int TextLen=-1;

void mqtt_callback(char* topic, byte* payload, unsigned int length) {
  String s;
  
  if (strstr(topic, "/config/ESP-") && strstr(topic, "/roomname")) {
    if (length>=32) length=31;
    strncpy(MyRoom, (char *) payload, length);
    MyRoom[length]='\0';
    SerialDebug("New Room-Name: ");
    SerialDebugln(MyRoom);

    s = "/room/"; s+=MyRoom; s+="/"; s+=MyHostname;
    mqttClient.publish(s.c_str(), MyIp, true);

    s = "/room/"; s+=MyRoom; s+="/wordclock/color";
    mqttClient.subscribe(s.c_str());
    s = "/room/"; s+=MyRoom; s+="/wordclock/mode";
    mqttClient.subscribe(s.c_str());
    s = "/room/"; s+=MyRoom; s+="/wordclock/tz";
    mqttClient.subscribe(s.c_str());
    s = "/room/"; s+=MyRoom; s+="/wordclock/text";
    mqttClient.subscribe(s.c_str());
  } else if (strstr(topic, "/room/") && strstr(topic, "/wordclock/color")) {
    char col[8];
    if (length>=8) length=7;
    strncpy(col, (char *) payload, length);
    col[length]='\0';
    int r,g,b;
    b=strtol(&(col[5]), NULL, 16);
    col[5]='\0';
    g=strtol(&(col[3]), NULL, 16);
    col[3]='\0';
    r=strtol(&(col[1]), NULL, 16);
    color = RgbColor(r,g,b);
  } else if (strstr(topic, "/room/") && strstr(topic, "/wordclock/mode")) {
    switch (payload[0]) {
      case '1': mode=1; break;
      case '2': mode=2; break;
    }
  } else if (strstr(topic, "/room/") && strstr(topic, "/wordclock/tz")) {
    char off[6];
    if (length>=6) length=5;
    strncpy(off, (char *) payload, length);
    off[length]='\0';
    timeClient.setTimeOffset(atoi(off)*60);
  } else if (strstr(topic, "/room/") && strstr(topic, "/wordclock/text")) {
    payload[length-1]='\0';
    text=String((char *) payload);
  }
}

void mqtt_reconnect() {
  String s;
  
  while (!mqttClient.connected()) {
    s = "/config/"; s+=MyHostname; s+="/online";
    if (mqttClient.connect(MyHostname, s.c_str(), 0, true, "0")) {
      mqttClient.publish(s.c_str(), "1", true);
      
      s="/config/"; s+=MyHostname; s+="/ipaddr";
      mqttClient.publish(s.c_str(), MyIp, true);
      
      s="/config/"; s+=MyHostname; s+="/roomname";
      mqttClient.subscribe(s.c_str());
    } else {
      delay(5000);
    }
  }
}

void setup() {
  Serial.begin(9600);
  Serial.setTimeout(200);
  delay(100);

  WiFiManager wifiManager;
  wifiManager.setDebugOutput(debug);
  wifiManager.setTimeout(3*60);
  if(!wifiManager.autoConnect()) {
    delay(3000);
    ESP.reset();
    delay(5000);
  }
  IPAddress MyIP=WiFi.localIP();
  snprintf(MyIp, 16, "%d.%d.%d.%d", MyIP[0], MyIP[1], MyIP[2], MyIP[3]);
  snprintf(MyHostname, 15, "ESP-%08x", ESP.getChipId());
  SerialDebug("ESP-Hostname: ");
  SerialDebugln(MyHostname);
  
  MDNS.begin(MyHostname);
  int n = MDNS.queryService("mqtt", "tcp");
  SerialDebugln("mDNS query done");
  if (n == 0) {
    SerialDebugln("no services found");
  }
  else {
    SerialDebug(n);
    SerialDebugln(" service(s) found");
    for (int i = 0; i < n; ++i) {
      // Print details for each service found
      SerialDebug(i + 1);
      SerialDebug(": ");
      SerialDebug(MDNS.hostname(i));
      SerialDebug(" (");
      SerialDebug(MDNS.IP(i));
      SerialDebug(":");
      SerialDebug(MDNS.port(i));
      SerialDebugln(")");
      mqttClient.setServer(MDNS.IP(i), MDNS.port(i));
    }
  }
  SerialDebugln("");
  
  mqttClient.setCallback(mqtt_callback);

  timeClient.begin();
    
  strip.Begin();
  strip.Show();
}

byte RenderLetter(byte * pos, const char x) {
  byte l=0;
  for (int i=0 ; i<FontWidth ; i++) {
    if (font[x][i] != 0) {
      pos[l] = font[x][i];
      l++;
    }
  }
  return l;
}
void RenderText(const uint8_t * text, size_t len) {
  const uint8_t * p = text;
  TextLen=0;

  for (size_t i=0 ; i<len && i<MaxTextLen-8 ; i++) {
    TextLen += RenderLetter(&Text[TextLen], *p);
    Text[TextLen]=0; TextLen++;   // one row free space
    p++;
  }
}
void RenderText(const char * text) {
  RenderText((const uint8_t *) text, strlen(text));
}
void RenderText(String s){
  RenderText((const uint8_t *)s.c_str(), s.length());
}

void ShowText() {
  strip.ClearTo(black);
  for (uint8_t i=0 ; i<panelWidth ; i++) {
    if (TextPos+i >= TextLen) break;
    for (int x=0 ; x<8 ; x++) {
      if ((Text[TextPos+i] & (1<<x)) != 0) {
        strip.SetPixelColor(MAP(i+1,9-x), color);
      }
    }
  }
  strip.Show();
}

void setWord(uint8_t line, uint8_t start, uint8_t cnt) {
  for (uint8_t i=start ; i<start+cnt ; i++)
    strip.SetPixelColor(MAP(i, line), color);
}

void ShowTime() {
  uint8_t m,h;
  
  strip.ClearTo(black);
  strip.SetPixelColor(MAP(1, 1), color);     // ES
  strip.SetPixelColor(MAP(2, 1), color);
  strip.SetPixelColor(MAP(4, 1), color);     // IST
  strip.SetPixelColor(MAP(5, 1), color);
  strip.SetPixelColor(MAP(6, 1), color);

  m=timeClient.getMinutes()%5;      // minute dots
  if (m>=1) strip.SetPixelColor(1, color);
  if (m>=2) strip.SetPixelColor(pixelCount-1, color);
  if (m>=3) strip.SetPixelColor(0, color);
  if (m>=4) strip.SetPixelColor(pixelCount-2, color);

  m=timeClient.getMinutes()/5; h=0;
  switch(m) {
    case 0: setWord(10, 9 ,3);    // UHR
      break;
    case 1: setWord( 1, 8, 4);    // FÜNF
            setWord( 4, 2, 4);    // NACH
      break;
    case 2: setWord( 2, 1, 4);    // ZEHN
            setWord( 4, 3, 4);    // NACH
      break;
    case 3: setWord( 3, 5, 7);    // VIERTEL
            if (mode==1)
              setWord(4, 3, 4);   // NACH
            if (mode==2)
              h=1;
      break;
    case 4: setWord( 2, 5, 7);    // ZWANZIG
            setWord( 4, 3, 4);    // NACH
      break;
    case 5: setWord( 1, 8, 4);    // FÜNF
            setWord( 4, 7, 3);    // VOR
            setWord( 5, 1, 4);    // HALB
            h=1;
      break;
    case 6: setWord( 5, 1, 4);    // HALB
            h=1;
      break;
    case 7: setWord( 1, 8, 4);    // FÜNF
            setWord( 4, 3, 4);    // NACH
            setWord( 5, 1, 4);    // HALB
            h=1;
      break;
    case 8: setWord( 2, 5, 7);    // ZWANZIG
            setWord( 4, 7, 3);    // VOR
            h=1;
      break;
    case 9: if (mode==2)
              setWord(3, 1, 4);   // DREI-
            setWord( 3, 5, 7);    // VIERTEL
            if (mode==1)
              setWord(4, 7, 3);   // VOR
            h=1;
      break;
    case 10:setWord( 2, 1, 4);    // ZEHN
            setWord( 4, 7, 3);    // VOR
            h=1;
      break;
    case 11:setWord( 1, 8, 4);    // FÜNF
            setWord( 4, 7, 3);    // VOR
            h=1;
      break;
  }

  h += timeClient.getHours();
  switch (h) {
    case 0:
    case 12:
    case 24:setWord(5, 6, 5);     // ZWÖLF
      break;
    case 1:
    case 13:setWord( 6, 3, 3);    // EIN
            if (m!=0)
              setWord(6, 3, 4);   // EINS
      break;
    case 2:
    case 14:setWord(6, 1, 4);     // ZWEI
      break;
    case 3:
    case 15:setWord(7, 2, 4);     // DREI
      break;
    case 4:
    case 16:setWord(8, 8, 4);     // VIER
      break;
    case 5:
    case 17:setWord(7, 8, 4);     // FÜNF
      break;
    case 6:
    case 18:setWord(10, 2, 5);    // SECHS
      break;
    case 7:
    case 19:setWord(6, 6, 6);     // SIEBEN
      break;
    case 8:
    case 20:setWord(9, 2, 4);     // ACHT
      break;
    case 9:
    case 21:setWord(8, 4, 4);     // NEUN
      break;
    case 10:
    case 22:setWord(9, 6, 4);     // ZEHN
      break;
    case 11:
    case 23:setWord(8, 1, 3);     // ELF
      break;
  }

  strip.Show();
}

unsigned long last=0;
unsigned int step=1000;
void loop() {
  if (!mqttClient.connected()) {
    mqtt_reconnect();
  }
  mqttClient.loop();

  timeClient.update();
  timeClient.getEpochTime();
  
  unsigned long cur=millis();
  if (cur-last >= step) {
    last=cur;
    if (text.length() > 0) {
      RenderText(text);
      TextPos=0;
      text="";
    }
    if (TextPos<=TextLen) {  
      ShowText();
      TextPos++;
      step=200;
    } else {
      ShowTime();
      step=1000;
    }
  }
}
