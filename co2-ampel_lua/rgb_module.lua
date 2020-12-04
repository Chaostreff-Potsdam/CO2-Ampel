module = {}

print("Load RGB")

ws2812.init()

local maxled = 12
local bright = 50

buff = ws2812.newBuffer(maxled, 3)
buffer_white = ws2812.newBuffer(maxled, 3)
buffer_green = ws2812.newBuffer(maxled, 3)
buffer_yellow = ws2812.newBuffer(maxled, 3)
buffer_red = ws2812.newBuffer(maxled, 3)

buff:fill(0, 0, 0, 10)

buffer_white:fill(bright, bright, bright)
buffer_green:fill(bright, 0, 0)
buffer_red:fill(0, bright, 0)
ws2812.write(buff) -- turn the two first RGBW leds to white

function round(a)
    return 
end

function led_set()
    local led_val = co2_median
    local rd = 0
    local gn = 0
    if (type(led_val) == "number") then
        if led_val < 800 then
            buff:mix(256, buffer_green)
        elseif led_val < 1000 then
            rd = math.ceil((led_val - 800) / 200 * 256)
            gn = 256 - rd
            buff:mix(gn, buffer_green, rd, buffer_red)
        else
            buff:mix(256, buffer_red)
        end
        
        for i = maxled,1,-1 do
            if (i-1 > (led_val / 100)) then
                buff:set(i, {0,0,0})
            elseif (i > (led_val / 100)) then
                ye = led_val % 100 / 100
                if led_val < 800 then
                    buff:set(i, {ye*bright, 0, 0})
                elseif led_val < 1000 then
                    buff:set(i, {ye*gn/256*bright, ye*rd/256*bright, 0})
                else
                    buff:set(i, {0, ye*bright, 0})
                end
            end
        end
    end
    if (wifi.sta.status() == 5) then
        buff:set(1, {0, 0, bright, 0})
    end
 --   print(buff)
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
