local module = {}
m = nil

print("Loading MH-Z19 CO2 Sensor app")

local led_state     = false

local lowDuration   = 0
local highDuration  = 0
local lastTimestamp = 0

local ageMaxValDay  = 0
local ageMaxValHour = 0
local MaxValDay     = 0
local MaxValHour    = 0

tmr4 = tmr.create()
tmr5 = tmr.create()
tmr6 = tmr.create()

local latestMeasurements = {}

local function wemos_d1_toggle_led()
    led_state = not led_state
    if led_state then
        gpio.write(config.HW.LED_PIN, gpio.HIGH)
    else
        gpio.write(config.HW.LED_PIN, gpio.LOW)
    end
end

local function mhz19_calculate_value(highDuration, lowDuration)
    val = config.HW.MZZ19_MAX * (1002.0 * highDuration - 2.0 * lowDuration) / 1000.0 / (highDuration + lowDuration);
    -- print(val)
    return val
end

local function mhz19InterruptHandler(level, timestamp)
    if (level == gpio.LOW) then
        highDuration = timestamp - lastTimestamp
    else
        lowDuration = timestamp - lastTimestamp
        local co2 = mhz19_calculate_value(highDuration, lowDuration)
        table.insert(latestMeasurements, co2)
    end
    lastTimestamp = timestamp
end

local function mhz19_median_value()
        
    -- get a median of the latest CO2 readings
    local measurements = latestMeasurements
    latestMeasurements = {}
    if (#measurements > 0) then
        table.sort(measurements)
        local median = measurements[math.ceil(#measurements / 2 + 1)]
        return median
    else
        return nil
    end
end

-- Updates and gets the maximum value for the last 24h
local function mhz19_maxval_day(curval)

    local timeNow = tmr.time()
    
    if (ageMaxValDay == 0) then
        -- First sample
        ageMaxValDay = timeNow
        MaxValDay    = curval
    end

    if (curval ~= nil and (timeNow - ageMaxValDay > 24 * 3600 or curval > MaxValDay or timeNow < ageMaxValHour)) then
        ageMaxValDay = timeNow
        MaxValDay    = curval
    end
    
    return MaxValDay
end

-- Updates and gets the maximum value for the last hour
local function mhz19_maxval_hour(curval)

    local timeNow = tmr.time()

    if (ageMaxValHour == 0) then
        -- First sample
        ageMaxValHour = timeNow
        MaxValHour    = curval
    end

    if (curval ~= nil and (timeNow - ageMaxValHour > 3600 or curval > MaxValHour or timeNow < ageMaxValHour)) then
        ageMaxValHour = timeNow
        MaxValHour    = curval
    end
    
    return MaxValHour
end

-- Sends a simple ping to the broker
local function mqtt_send_ping()
    local rssi       = wifi.sta.getrssi()
    local median     = mhz19_median_value()
    local maxvalDay  = mhz19_maxval_day(median)
    local maxvalHour = mhz19_maxval_hour(median)
    
    -- m:publish(config.MQTT.ENDPOINT .. "id", config.MQTT.ID,0,0)
    val = median or "null"
    m:publish(config.MQTT.ENDPOINT, "{ \"ppmCO2\": " .. val .. " }",0,0)
    -- m:publish(config.MQTT.ENDPOINT .. "value/maxday", maxvalDay or "null",0,0)
    -- m:publish(config.MQTT.ENDPOINT .. "value/maxhour", maxvalHour or "null",0,0)
    -- m:publish(config.MQTT.ENDPOINT .. "unit", "ppm",0,0)
    -- m:publish(config.MQTT.ENDPOINT .. "rssi", rssi,0,0)
    print("id:", config.MQTT.ID, "; rssi:", rssi, "dBm; CO2 median:", median,"ppm", "; daymax:", maxvalDay,"ppm","; hourmax:", maxvalHour,"ppm" )
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

local function after_warmup_mhz19()
  mhz19_attach_interrupt()
  listen_telnet()
end

local function listen_telnet()
  -- Start the telnet server
  telnet:open(nil, nil, config.TELNET.PORT)
end

local function mhz19_attach_interrupt()
  gpio.mode(config.HW.MHZ19_PIN, gpio.INT)
  gpio.trig(config.HW.MHZ19_PIN, "both", mhz19InterruptHandler)
  tmr4:stop()
  gpio.write(config.HW.LED_PIN, gpio.HIGH)
  print("MH-Z19 is ready")
end

function module.start()
  --get reset reason. If we powered on then we need to wait 120 seconds for
  --the MH-Z19 to be warmed up
  _, reset_reason = node.bootreason()
  
  print("Running app... Reset reason = ", reset_reason)
  
  -- configure LED
  gpio.mode(config.HW.LED_PIN, gpio.OUTPUT)
  gpio.write(config.HW.LED_PIN, gpio.HIGH)
  
  -- blink reset reason
--  gpio.serout(config.HW.LED_PIN, gpio.LOW, {150000,50000},reset_reason+1, function()
    -- configure reading MH-Z19
if (reset_reason == 0 or reset_reason == 6) then
  print("Warming up sensor...")
  tmr5:alarm(120000, tmr.ALARM_SINGLE, mhz19_attach_interrupt)
  tmr4:alarm(200, tmr.ALARM_AUTO, wemos_d1_toggle_led)
else
  mhz19_attach_interrupt()
end

mqtt_start()
  
end

return module
