//#######################################################################################################
//#################################### Plugin 092: Nodo Event Bridge ####################################
//#######################################################################################################

#define PLUGIN_092
#define PLUGIN_ID_092         92
#define PLUGIN_NAME_092       "Nodo Event Bridge"
#define PLUGIN_VALUENAME1_092 ""

boolean Plugin_092(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;
  static byte ipOctet = 255;
  
  switch (function)
  {

    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_092;
        Device[deviceCount].Custom = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_092);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_092));
        break;
      }

    case PLUGIN_UDP_IN:
      {
        if (event->Data[0] == 255 && event->Data[1] == 254)
        {
          if (event->Data[2] != 255 && event->Data[3] != 255)
          {
            boolean valid = true;

            // if in sendto mode, block traffic from other IP node's
            if (ipOctet !=255 && event->Par1 !=ipOctet)
              valid = false;
              
            if (valid)
              for (byte x = 0; x < 16; x++)
                Serial.write(event->Data[x]);
          }
          else
          {
            ipOctet = event->Data[4];
          }
        }
        success = true;
        break;
      }

    case PLUGIN_SERIAL_IN:
      {
        if (Serial.peek() == 255)
        {
          delay(20); // wait for message to complete
          if (Serial.read() == 255 && Serial.read() == 254)
          {
            IPAddress ip = WiFi.localIP();
            IPAddress sendIP(ip[0], ip[1], ip[2], ipOctet);
            portTX.beginPacket(sendIP, Settings.UDPPort);
            byte data[16];
            data[0] = 255;
            data[1] = 254;
            byte count = 2;
            while (Serial.available() && count < 16)
              data[count++] = Serial.read();
            portTX.write(data, 16);
            portTX.endPacket();
          }
          success = true;
        }
        break;
      }

  }
  return success;
}

