-- configure Calibration
  gpio.mode(6, gpio.OUTPUT)
  gpio.write(6, gpio.HIGH)

if file.exists("eus_params.lua") then
    app    = require("app_mhz19")
    config = require("config")
    setup  = require("setup")
    mq     = require("mqtt_module")
    led    = require("rgb_module")

    app.start()
    led.start()
    setup.start()
else
    eus    = require("eus")
    eus.start()
 
    app    = require("app_mhz19")
    config = require("config")
    led    = require("rgb_module")

    app.start()
    led.start()
end

