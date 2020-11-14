module = {}

print("Load RGBW")

ws2812.init()

local maxled = 15

buffer = ws2812.newBuffer(maxled, 4)
local buffer_white = ws2812.newBuffer(maxled, 4)
local buffer_green = ws2812.newBuffer(maxled, 4)
local buffer_yellow = ws2812.newBuffer(maxled, 4)
local buffer_red = ws2812.newBuffer(maxled, 4)

buffer:fill(0, 0, 0, 255)
ws2812.write(buffer) -- turn the two first RGBW leds to white

buffer_white:fill(0, 0, 0, 127)
buffer_green:fill(255, 0, 0, 0)
buffer_red:fill(0, 255, 0, 0)
buffer_yellow:mix(127, buffer_green, 127, buffer_red)

function round(a)
    return 
end

function led_set()
    if (type(co2) == "number") then
        if co2 < 800 then
            buffer = buffer_green
        elseif co2 < 1000 then
            local rd = math.ceil((co2 - 800) / 200 * 255)
            local gn = 255 - rd
            print("rd:"..rd.."-gn:"..gn)
            buffer:mix(gn, buffer_green, rd, buffer_red)
        else
            buffer = buffer_red
        end
        
        for i = maxled,1,-1 do
            if ((co2 / 100) < i) then
                buffer:set(i, {0,0,0,0})
            end
        end
    print("led_set: " .. co2)
    end
    
    ws2812.init()
    ws2812.write(buffer) -- turn the two first RGBW leds to white

end

function led_start()
    ledtmr = tmr.create()
    ledtmr:register(5000, tmr.ALARM_AUTO, led_set)
    ledtmr:start()
end

function module.start()
    led_start()
end

return module