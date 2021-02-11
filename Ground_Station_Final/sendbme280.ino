void bme280()
{
  digitalWrite(LED, HIGH);
  uint8_t comd[] = "BME280";
  Serial.print("Transmitting "); Serial.println((char*)comd); // Send a message to rf95_server
  
  rf95.send(comd, sizeof(comd));
  delay(10);
  rf95.waitPacketSent();
  rx_s = "";
  digitalWrite(LED, LOW);
    
  // Now wait for a reply
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);

  Serial.println("Waiting for reply...");
  if (rf95.waitAvailableTimeout(3000))
  { 
    // Should be a reply message for us now   
    if (rf95.recv(buf, &len))
    {
      digitalWrite(LED, HIGH);
      u.b[0]=buf[0];
      u.b[1]=buf[1];
      u.b[2]=buf[2];
      u.b[3]=buf[3];
      Serial.print("Temperature: ");
      Serial.print(u.fval);
      Serial.print("°C, ");
      digitalWrite(LED, LOW);
      
      digitalWrite(LED, HIGH);
      u.b[0]=buf[4];
      u.b[1]=buf[5];
      u.b[2]=buf[6];
      u.b[3]=buf[7];
      Serial.print("Altitude: ");
      Serial.print(u.fval);
      Serial.print("m, ");
      digitalWrite(LED, LOW);
      
      digitalWrite(LED, HIGH);
      u.b[0]=buf[8];
      u.b[1]=buf[9];
      u.b[2]=buf[10];
      u.b[3]=buf[11];
      Serial.print("Humidity: ");
      Serial.print(u.fval);
      Serial.print("%, ");
      digitalWrite(LED, LOW);
      
      digitalWrite(LED, HIGH);
      u.b[0]=buf[12];
      u.b[1]=buf[13];
      u.b[2]=buf[14];
      u.b[3]=buf[15];
      Serial.print("Pressure: ");
      Serial.print(u.fval);
      Serial.print("Pa, ");
      digitalWrite(LED, LOW);
      
      Serial.print("RSSI: ");
      Serial.println(rf95.lastRssi(), DEC);
    }
    else
    {
      Serial.println("Receive failed");
    }
  }
  else
  {
    Serial.println("Unable to receive BME280 data.");
  }
}
