if file.exists("eus_params.lua") then
    app    = require("app_mhz19")
    config = require("config")
    setup  = require("setup")
    mq     = require("mqtt_module")
    led    = require("rgbw_module")

    app.start()
    led.start()
    setup.start()
else
    eus    = require("eus")
    eus.start()
 
    app    = require("app_mhz19")
    config = require("config")
    led    = require("rgbw_module")

    app.start()
    led.start()
end

