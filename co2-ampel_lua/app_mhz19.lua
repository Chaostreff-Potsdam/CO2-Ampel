local module = {}

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

co2 = 0

local latestMeasurements = {}
co2_median = 0

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
    -- print("{ \"ppmCO2\": " .. val .. " }")
    return val
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

local function mhz19InterruptHandler(level, timestamp)
--print(timestamp .. "-" .. lastTimestamp)
    if (level == gpio.LOW) then
        highDuration = timestamp - lastTimestamp
    else
        if (lastTimestamp == 0) then
            co2 = nil
        else
            lowDuration = timestamp - lastTimestamp
            co2 = mhz19_calculate_value(highDuration, lowDuration)
            -- local new = mhz19_calculate_value(highDuration, lowDuration)
            table.insert(latestMeasurements, co2)
            if (#latestMeasurements > 4) then
                co2_median = mhz19_median_value()
            end
            -- print(#latestMeasurements)
            -- print(co2)
        end
    end
    lastTimestamp = timestamp
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

local function after_warmup_mhz19()
  mhz19_attach_interrupt()
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
  
  -- configure Calibration
  gpio.mode(config.HW.CAL_PIN, gpio.OUTPUT)
  gpio.write(config.HW.CAL_PIN, gpio.HIGH)


  -- blink reset reason
--  gpio.serout(config.HW.LED_PIN, gpio.LOW, {150000,50000},reset_reason+1, function()
    -- configure reading MH-Z19
    if (reset_reason == 0 or reset_reason == 6) then
      print("Warming up sensor...")
      tmr5:alarm(2000, tmr.ALARM_SINGLE, mhz19_attach_interrupt)
      tmr4:alarm(200, tmr.ALARM_AUTO, wemos_d1_toggle_led)
    else
      print("Starting sensor...")
      mhz19_attach_interrupt()
    end

end

return module
