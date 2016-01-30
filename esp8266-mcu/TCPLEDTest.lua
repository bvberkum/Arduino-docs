
wifi.setmode(wifi.STATION)
wifi.sta.config("Wirez", "9n753bskrudn210dg127fdlr")

gpio.mode(4, gpio.OUTPUT)

port = 43333

srv=net.createServer(net.TCP) 
srv:listen(port, function(conn)
    conn:on("receive",function(conn,payload)
        print("Input:"..payload) 
        if string.find(payload, "OFF") then
            gpio.write(4, gpio.HIGH)
        elseif string.find(payload, "ON") then
            gpio.write(4, gpio.LOW)
        end
    end)
end)

print ("Booted to TCPLEDTest.0")

print ("Listening, port "..port)
print (wifi.sta.getmac())
print (wifi.sta.getip())
