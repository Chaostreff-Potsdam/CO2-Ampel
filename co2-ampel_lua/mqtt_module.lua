local module = {}
print("Load MQTT")
m = nil

-- Sends a simple ping to the broker
local function mqtt_send_ping()
    local rssi       = wifi.sta.getrssi()
--    local median     = app.mhz19_median_value()
--    local maxvalDay  = app.mhz19_maxval_day(median)
--    local maxvalHour = app.mhz19_maxval_hour(median)
    
    -- m:publish(config.MQTT.ENDPOINT .. "id", config.MQTT.ID,0,0)
    val = co2 or "null"
    m:publish(config.MQTT.ENDPOINT, "{ \"ppmCO2\": " .. val .. " }",0,0)
    -- m:publish(config.MQTT.ENDPOINT .. "value/maxday", maxvalDay or "null",0,0)
    -- m:publish(config.MQTT.ENDPOINT .. "value/maxhour", maxvalHour or "null",0,0)
    -- m:publish(config.MQTT.ENDPOINT .. "unit", "ppm",0,0)
    -- m:publish(config.MQTT.ENDPOINT .. "rssi", rssi,0,0)
    print("id:", config.MQTT.ID, "; rssi:", rssi, "dBm; CO2 value:", val,"ppm")
end

-- Sends my id to the broker for registration
local function mqtt_register_myself()
    m:subscribe(config.MQTT.ENDPOINT .. config.MQTT.ID,0,function(conn)
        print("Successfully subscribed to data endpoint")
    end)
end

local function mqtt_stop()
    m:close()
    m = nil
end

local function mqtt_start()
print("Start MQTT")
    m = mqtt.Client(config.MQTT.ID, 120, config.MQTT.USER, config.MQTT.PASSWORD)
    -- register message callback beforehand
    m:on("message", function(conn, topic, data) 
      if data ~= nil then
        print(topic .. ": " .. data)
        -- do something, we have received a message
      end
    end)
    
    m:on("offline", function(conn)
        print("Mqtt client gone offline - reconnecting in 30 seconds")
        tmr6:stop()
        tmr6:alarm(30000, tmr.ALARM_SINGLE, function()
            mqtt_stop()
            mqtt_start()
        end)
    end)
    
    -- Connect to broker
    print(config.MQTT.HOST)
    m:connect(config.MQTT.HOST, config.MQTT.PORT, false, function(con) 
        print("connect")
        mqtt_register_myself()
        -- And then pings each 5000 milliseconds
    print("SendPings")
        tmr6:stop()
        tmr6:alarm(5000, tmr.ALARM_AUTO, mqtt_send_ping)
    end, function(con,reason)
        print("Connection failed with reason: " .. reason .. " - reconnecting in 30 seconds")
        tmr6:stop()
        tmr6:alarm(30000, tmr.ALARM_SINGLE, function()
            mqtt_stop()
            mqtt_start()
        end)
    end) 

end

function module.start()
mqtt_start()
end

return module