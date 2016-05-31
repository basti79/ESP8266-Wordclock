# Protocol

## Hostname
Each module generates a "hostname" starting with "ESP-" followed by the 8-digit HEX chip-id. This is used to identify a new unique Module in the MQTT-Tree.

## Default MQTT-Topics
* **/config/<hostname>/online:** 1 - if module in online, 0 - if not (generated through "will message")
* **/config/<hostname>/ipaddr:** IP address of the module
* **/config/<hostname>/roomname:** from here the modules reads its "room name", you whould publish this as retained message

The following topics are only published if the room name is set:
* **/room/<roomname>/<hostname>:** IP address of the module
* **/room/<roomname>/wordclock/color:** sets the color of the clock in hex "#rrggbb"
* **/room/<roomname>/wordclock/mode:** sets the mode (1 - "viertel nach"/"viertel vor", 2 - "viertel"/"dreiviertel")
* **/room/<roomname>/wordclock/tz:** sets the timezone, specify offset to UTC in minutes
* **/room/<roomname>/wordclock/text:** display the given text once (attention: last char is cut of and size is limited due to limitation in PubSubClient)
