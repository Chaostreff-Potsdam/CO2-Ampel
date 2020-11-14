module = {}
-- Starts the portal to choose the wi-fi router to which we have
-- to associate
function module.start()
    wifi.sta.disconnect()
    wifi.setmode(wifi.STATIONAP)
    --ESP SSID generated wiht its chipid
    wifi.ap.config({ssid="CO2Ampel-"..node.chipid()
    , auth=wifi.OPEN})
    print("Start EUS")
    enduser_setup.manual(true)
    enduser_setup.start(
      function()
        print("Connected to wifi as:" .. wifi.sta.getip())
      end,
      function(err, str)
        print("enduser_setup: Err #" .. err .. ": " .. str)
      end
    );
end