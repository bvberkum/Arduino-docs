pwm.setup(3, 1000, 005)
pwm.setup(4, 1000, 005)
pwm.start(3)
pwm.start(4)

LED1_current=005
LED1_target=005
LED2_current=005
LED2_target=005

Fadetime1=5000
Fadetime2=5000

Stepcounter1=0
PosStepcounter1=0
DimTimer1=0

Stepcounter2=0
PosStepcounter2=0
DimTimer2=0

wifi.setmode(wifi.STATION)
wifi.sta.config("Wirez","9n753bskrudn210dg127fdlr")



srv=net.createServer(net.TCP) 
srv:listen(43333,function(conn) 
    conn:on("receive",function(conn,payload) 
   
    print("Input:"..payload) 
 
    if string.find(payload,"LED1") then
     LED1_target=tonumber(string.sub(payload, 13) )
     print("Received LED1 Target Value: "..LED1_target)

     Stepcounter1=(LED1_target)-(LED1_current)
     
     if (Stepcounter1) < 0 then
      PosStepcounter1=(Stepcounter1)*-1
      else PosStepcounter1=(Stepcounter1)
     end
     
     if (PosStepcounter1) == 0 then
      PosStepcounter1=(PosStepcounter1)+1
      else PosStepcounter1=(PosStepcounter1)
     end
          
     DimTimer1=(Fadetime1)/(PosStepcounter1)

     if (DimTimer1) == 0 then 
      DimTimer1=(DimTimer1)+1
      else DimTimer1=(DimTimer1)
     end

      print (Fadetime1)
      print (Stepcounter1)
      print (PosStepcounter1)
      print (DimTimer1)
      print (LED1_current)
      print (LED1_target)


    tmr.alarm(0, (DimTimer1), 1, function() 
     if LED1_current < LED1_target then 
      LED1_current = (LED1_current + 1) 
      pwm.setduty(3, LED1_current)
    elseif LED1_current > LED1_target then 
      LED1_current = (LED1_current - 1) 
      pwm.setduty(3, LED1_current)
    elseif LED1_current == LED1_target then tmr.stop(0)
     end end )
    end

    if string.find(payload,"LED2") then
        print("Received LED2 Target Value")
     LED2_target=tonumber(string.sub(payload, 13) )
     
     Stepcounter2=(LED2_target)-(LED2_current)
     
     if (Stepcounter2) < 0 then
      PosStepcounter2=(Stepcounter2)*-1
      else PosStepcounter2=(Stepcounter2)
     end
     
     if (PosStepcounter2) == 0 then
      PosStepcounter2=(PosStepcounter2)+1
      else PosStepcounter2=(PosStepcounter2)
     end
          
     DimTimer2=(Fadetime2)/(PosStepcounter2)

     if (DimTimer2) == 0 then 
      DimTimer2=(DimTimer2)+1
      else DimTimer2=(DimTimer2)
     end

      print (Fadetime2)
      print (Stepcounter2)
      print (PosStepcounter2)
      print (DimTimer2)
      print (LED2_current)
      print (LED2_target)


    tmr.alarm(1, (DimTimer2), 1, function() 
     if LED2_current < LED2_target then 
      LED2_current = (LED2_current + 1) 
      pwm.setduty(4, LED2_current)
    elseif LED2_current > LED2_target then 
      LED2_current = (LED2_current - 1) 
      pwm.setduty(4, LED2_current)
    elseif LED2_current == LED2_target then tmr.stop(1)
     end end )
    end
    end)
    end)

print ("Booted to QuinLED_ESP8266_V0.4")