# CO2 Traffic Light

> :warning: Please also see `refactoring-arduino` branch. I have no hardware at
hand to test the implementation of the refactoring branch.

This code uses the Arduino framework for the ESP8266. To build and flash the
firmware use [PlatformIO](https://platformio.org/). It should handle the
dependencies.

## Used Hardware

- D1 Mini (but any board with an ESP8266/ESP32 should do the job)
- MH-Z19(B) CO2 Sensor
- NeoPixel Ring with 12 LEDs

## Wiring

No fancy image here.

- D1 Mini `D7` -> MH-Z19(B) `TX`
- D1 Mini `D8` -> MH-Z19(B) `RX`
- D1 Mini `D5` -> NeoPixel `Data`
- D1 Mini `D3` -> Button to `GND` 

## Configuration

1. copy the file [src/mqtt_config.example.h](src/mqtt_config.example.h) to
`src/mqtt_config.h` and adjust the values
1. copy the file [src/wifi_config.example.h](src/wifi_config.example.h) to
`src/wifi_config.h` and adjust the values

Pin configuration can be changed in the [src/main.cpp](src/main.cpp).

Instead of a NeoPixel ring a compatible strip can be used. The number of LEDs
should be set accordingly.
