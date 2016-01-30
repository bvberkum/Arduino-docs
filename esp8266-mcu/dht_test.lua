
PIN = 4 --  data pin, GPIO2

DHT= require("dht_lib")

DHT.read(PIN)

t = DHT.getTemperature()
h = DHT.getHumidity()

if h == nil then
  print("Error reading from DHTxx")
else
  -- temperature in degrees Celsius  and Farenheit

print("Temperature: "..t.." deg C")

  -- humidity

  print("Humidity: "..((h - (h % 10)) / 10).."."..(h % 10).."%")
end

-- release module
DHT = nil
package.loaded["dht_lib"]=nil
