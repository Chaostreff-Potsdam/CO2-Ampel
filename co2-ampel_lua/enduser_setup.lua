local module = {}

function module.start()
print("start_eus")
    --wifi.setmode(wifi.STATIONAP)
    --wifi.ap.config({ssid="CO2_Ampel", auth=wifi.OPEN})
    enduser_setup.manual(false)
    enduser_setup.start(
      function()
        print("Connected to WiFi as:" .. wifi.sta.getip())
      end,
      function(err, str)
        print("enduser_setup: Err #" .. err .. ": " .. str)
      end
    )
end

return module
--FileView done.
