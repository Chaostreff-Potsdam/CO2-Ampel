local module = {}

print("Load EUS")

function module.start()
print("start_eus")
    --wifi.setmode(wifi.STATIONAP)
    --wifi.ap.config({ssid="CO2_Ampel", auth=wifi.OPEN})

print("Wifi device_name: " .. p.device_name)
    enduser_setup.manual(false)
    enduser_setup.start(
      function()
        print("Connected to WiFi as:" .. wifi.sta.getip())
        -- now use the parameters in the Lua table
        module.p = dofile('eus_params.lua')

      end,
      function(err, str)
        print("enduser_setup: Err #" .. err .. ": " .. str)
      end
    )
end

return module
--FileView done.
