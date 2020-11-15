local module = {}
-- Starts the portal to choose the wi-fi router to which we have
-- to associate
print("Loading EUS")

function module.start()
    if file.exists("eus_params.lua") then
        wlan = dofile("eus_params.lua")
        station_cfg = {}
        station_cfg.ssid = wlan.wifi_ssid
        station_cfg.pwd = wlan.wifi_password
        wifi.sta.config(station_cfg)
        wifi.sta.connect()
        return 
    else
        print("Start EUS")
        wifi.sta.disconnect()
        wifi.setmode(wifi.STATIONAP, true)
        --ESP SSID generated wiht its chipid
        wifi.ap.config({ssid="CO2Ampel-"..node.chipid(), auth=wifi.OPEN})
        enduser_setup.manual(true)
        enduser_setup.start(
          function()
            print("Connected to wifi as:" .. wifi.sta.getip())
            node.reset()
          end,
          function(err, str)
            print("enduser_setup: Err #" .. err .. ": " .. str)
          end
        );
    end
    return
end

function module.stop()
    enduser_setup.stop()
end

return module
