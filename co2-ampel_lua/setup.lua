local module = {}

--[[
 This module sets up a WiFi connection and launches the app as soon as it's ready
 This code is derived from the NodeMCU Docs https://nodemcu.readthedocs.io/en/master/en/upload/#initlua
 as well as the tutorial on https://www.foobarflies.io/a-simple-connected-object-with-nodemcu-and-mqtt/
]] 
mytimer = tmr.create()

local function wifi_wait_ip()
  if wifi.sta.getip()== nil then
    print("IP unavailable, Waiting...")
  else
    mytimer:stop()
    print("\n====================================")
    print("ESP8266 mode is: " .. wifi.getmode())
    print("MAC address is: " .. wifi.ap.getmac())
    print("IP is "..wifi.sta.getip())
    print("====================================")
    app.start()
  end
end

local function wifi_start(list_aps)
    if list_aps then
        for key,value in pairs(list_aps) do
            if config.SSID and config.SSID[key] then
                wifi.setmode(wifi.STATION);
                wifi.sta.config({ssid=key, pwd=config.SSID[key]})
                -- wifi.sta.connect()
                print("Connecting to " .. key .. " ...")
                --config.SSID = nil  -- can save memory
                mytimer:alarm(2500, tmr.ALARM_AUTO, wifi_wait_ip)

--tmr.alarm(1, 2500, tmr.ALARM_AUTO, wifi_wait_ip)
            end
        end
    else
        print("Error getting AP list")
    end
end

-- Define WiFi station event callbacks 
local function wifi_connect_event(T) 
  print("Connection to AP("..T.SSID..") established!")
  print("Waiting for IP address...")
  if disconnect_ct ~= nil then disconnect_ct = nil end  
end

local function wifi_got_ip_event(T) 
  -- Note: Having an IP address does not mean there is internet access!
  -- Internet connectivity can be determined with net.dns.resolve().    
  print("Wifi connection is ready! IP address is: "..T.IP)
  print("Startup will resume momentarily, you have 3 seconds to abort.")
  print("Waiting...") 
  -- tmr.create():alarm(3000, tmr.ALARM_SINGLE, startup)
end

local function wifi_disconnect_event(T)
  if T.reason == wifi.eventmon.reason.ASSOC_LEAVE then 
    --the station has disassociated from a previously connected AP
    return 
  end
  -- total_tries: how many times the station will attempt to connect to the AP. Should consider AP reboot duration.
  local total_tries = 75
  print("\nWiFi connection to AP("..T.SSID..") has failed!")

  --There are many possible disconnect reasons, the following iterates through 
  --the list and returns the string corresponding to the disconnect reason.
  for key,val in pairs(wifi.eventmon.reason) do
    if val == T.reason then
      print("Disconnect reason: "..val.."("..key..")")
      break
    end
  end

  if disconnect_ct == nil then 
    disconnect_ct = 1 
  else
    disconnect_ct = disconnect_ct + 1 
  end
  if disconnect_ct < total_tries then 
    print("Retrying connection...(attempt "..(disconnect_ct+1).." of "..total_tries..")")
  else
    wifi.sta.disconnect()
    print("Aborting connection to AP!")
    disconnect_ct = nil  
  end
end

function module.start()
  print("Configuring Wifi ...")
  -- Register WiFi Station event callbacks
  -- wifi.eventmon.register(wifi.eventmon.STA_CONNECTED, wifi_connect_event)
  -- wifi.eventmon.register(wifi.eventmon.STA_GOT_IP, wifi_got_ip_event)
  -- wifi.eventmon.register(wifi.eventmon.STA_DISCONNECTED, wifi_disconnect_event)
  
  wifi.setmode(wifi.STATION);
  wifi.sta.getap(wifi_start);
end

return module
