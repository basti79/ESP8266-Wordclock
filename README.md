# ESP8266-Wordclock

This is an implementation of the famous Wordclock on an ESP8266 using
WS2812B LED-Stripes. The time is syncronized via NTP and can be configured
via MQTT for interagation with me other home automation projects.

It's based on the Arduino [Arduino core for ESP8266](https://github.com/esp8266/Arduino) and needs these libraries:
* [WiFiManager](https://github.com/tzapu/WiFiManager)
* [Arduino Client for MQTT](https://github.com/knolleary/pubsubclient)
* [NTPClient](https://github.com/arduino-libraries/NTPClient) (user git version, release 3.0.0 is missing "setTimeOffset")
* [NeoPixelBus](https://github.com/Makuna/NeoPixelBus)

Please use "git clone --recursive" to get the LCD-Fonts.

For more information an my hardware design see:
https://www.basti79.de/mediawiki/index.php/Wordclock

