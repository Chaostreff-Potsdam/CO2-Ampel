# esp8266-mhz19-mqtt

This is a LUA program for ESP8266 publishing MH-Z19 CO2 measurements via MQTT to a broker.

# Prerequisites

 - A ESP8266 board
 - A MH-Z19 CO2 sensor module
 - Serial connection to ESP8266

# Hardware 

For example a WeMos D1 Mini can be used as it is very convenient to work with. It offers 32 Mbits of flash memory and has got a
serial to usb converter.

![Wiring diagram](img/wiring.png?raw=true "Wiring diagram")

# Uploading the sources

# Acknowledgments

 - git@github.com:sonntam/esp8266-mhz19-mqtt.git
 - LUA dump function for data structures from https://stackoverflow.com/a/27028488
 - Documentation on init.lua from https://nodemcu.readthedocs.io/en/master/en/upload/#initlua
 - A simple connected object with NodeMCU and MQTT tutorial on https://www.foobarflies.io/a-simple-connected-object-with-nodemcu-and-mqtt/

# NodeMCU-Modules:
adc,bme680,dht,enduser_setup,file,gpio,mqtt,net,node,sjson,softuart,tmr,uart,websocket,wifi,ws2812,ws2812_effects

