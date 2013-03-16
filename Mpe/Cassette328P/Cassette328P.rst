

local
  date:Date:rw
  time:Date:rw
  coretemp:Temperature(r)
  casetemp:Temperature(r, '2C')
  casehum:Humidity(r, '5%')
  rf12:RF12

modes
  reporter
    rf12, serial
  repeater
    rf12, 
    

