void spectral()
{
  digitalWrite(LED, HIGH);
  uint8_t comd[] = "Spectral";
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
      h.b2[0]=buf[0];
      h.b2[1]=buf[1];
      Serial.print("Temperature: ");
      Serial.print(h.halfb);
      Serial.print("°C, ");
      digitalWrite(LED, LOW);
      
      digitalWrite(LED, HIGH);
      h.b2[0]=buf[2];
      h.b2[1]=buf[3];
      Serial.print("Violet: ");
      Serial.print(h.halfb);
      Serial.print(", ");
      digitalWrite(LED, LOW);
      
      digitalWrite(LED, HIGH);
      h.b2[0]=buf[4];
      h.b2[1]=buf[5];
      Serial.print("Blue: ");
      Serial.print(h.halfb);
      Serial.print(", ");
      digitalWrite(LED, LOW);
      
      digitalWrite(LED, HIGH);
      h.b2[0]=buf[6];
      h.b2[1]=buf[7];
      Serial.print("Green: ");
      Serial.print(h.halfb);
      Serial.print(", ");
      digitalWrite(LED, LOW);
      
      digitalWrite(LED, HIGH);
      h.b2[0]=buf[8];
      h.b2[1]=buf[9];
      Serial.print("Yellow: ");
      Serial.print(h.halfb);
      Serial.print(", ");
      digitalWrite(LED, LOW);
      
      digitalWrite(LED, HIGH);
      h.b2[0]=buf[10];
      h.b2[1]=buf[11];
      Serial.print("Orange: ");
      Serial.print(h.halfb);
      Serial.print(", ");
      digitalWrite(LED, LOW);
      
      digitalWrite(LED, HIGH);
      h.b2[0]=buf[12];
      h.b2[1]=buf[13];
      Serial.print("Red: ");
      Serial.print(h.halfb);
      Serial.print(", ");
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
