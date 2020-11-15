module = {}

print("Load RGBW")

ws2812.init()

local maxled = 15

buff = ws2812.newBuffer(maxled, 4)
buffer_white = ws2812.newBuffer(maxled, 4)
buffer_green = ws2812.newBuffer(maxled, 4)
buffer_yellow = ws2812.newBuffer(maxled, 4)
buffer_red = ws2812.newBuffer(maxled, 4)

buff:fill(0, 0, 0, 10)

buffer_white:fill(0, 0, 0, 255)
buffer_green:fill(255, 0, 0, 0)
buffer_red:fill(0, 255, 0, 0)
ws2812.write(buff) -- turn the two first RGBW leds to white

function round(a)
    return 
end

function led_set()
    local rd = 0
    local gn = 0
    if (type(co2) == "number") then
        if co2 < 800 then
            buff:mix(256, buffer_green)
        elseif co2 < 1000 then
            rd = math.ceil((co2 - 800) / 200 * 255)
            gn = 255 - rd
            buff:mix(gn, buffer_green, rd, buffer_red)
        else
            buff:mix(256, buffer_red)
        end
        
        for i = maxled,1,-1 do
            if (i-1 > (co2 / 100)) then
                buff:set(i, {0,0,0,0})
            elseif (i > (co2 / 100)) then
                ye = co2 % 100 / 100
                if co2 < 800 then
                    buff:set(i, {ye*255, 0, 0, 0})
                elseif co2 < 1000 then
                    buff:set(i, {ye*gn, ye*rd, 0, 0})
                else
                    buff:set(i, {0, ye*255, 0, 0})
                end
            end
        end
    end
    ws2812.init()
    ws2812.write(buff) -- turn the two first RGBW leds to white

end

function led_start()
    ledtmr = tmr.create()
    ledtmr:register(1000, tmr.ALARM_AUTO, led_set)
    ledtmr:start()
end

function module.start()
    led_start()
end

return module
