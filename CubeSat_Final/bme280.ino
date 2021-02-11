void bme280call()
{
  float tempc,alt,humid,pressure;
  tempc = mySensorA.readTempC();
  alt = mySensorA.readFloatAltitudeMeters();
  humid = mySensorA.readFloatHumidity();
  pressure = mySensorA.readFloatPressure();

  u.fval = tempc;
  bmedata[0]= u.b[0];
  bmedata[1]= u.b[1];
  bmedata[2]= u.b[2];
  bmedata[3]= u.b[3];
  
  u.fval = alt;
  bmedata[4]= u.b[0];
  bmedata[5]= u.b[1];
  bmedata[6]= u.b[2];
  bmedata[7]= u.b[3];
  
  u.fval = humid;
  bmedata[8]= u.b[0];
  bmedata[9]= u.b[1];
  bmedata[10]= u.b[2];
  bmedata[11]= u.b[3];
  
  u.fval = pressure;
  bmedata[12]= u.b[0];
  bmedata[13]= u.b[1];
  bmedata[14]= u.b[2];
  bmedata[15]= u.b[3];

  rf95.send(bmedata, sizeof(bmedata));
  rf95.waitPacketSent();
  delay(100);


  myFile = SD.open("bme820.txt", FILE_WRITE);

  // if the file opened okay, write to it:
  if (myFile) {
    Serial.print("Writing to SD...");
    myFile.println("BME280 Sensor Data:");
    myFile.print("Temperature: ");
    myFile.print(mySensorA.readTempC(), 2);
    myFile.print(" degrees C  /  ");
    myFile.print(mySensorA.readTempF(), 2);
    myFile.println(" degrees F");
    myFile.print("Altitude: ");
    myFile.print(mySensorA.readFloatAltitudeMeters(), 2);
    myFile.print("m  /  ");
    myFile.print(mySensorA.readFloatAltitudeFeet(), 2);
    myFile.println("ft"); 
    myFile.print("%RH: ");
    myFile.print(mySensorA.readFloatHumidity(), 2);
    myFile.println(" %");
    myFile.print("Pressure: "); 
    myFile.print(mySensorA.readFloatPressure(), 2);
    myFile.println(" Pa");

    myFile.println();
    myFile.close();
    Serial.println("done.");
  } 
  else {
    // if the file didn't open, print an error:
    Serial.println("error opening bme820.txt");
  }
}

void bme280setup()
{
  mySensorA.settings.commInterface = I2C_MODE;
  mySensorA.settings.I2CAddress = 0x77;
  mySensorA.settings.runMode = 3; //  3, Normal mode
  mySensorA.settings.tStandby = 0; //  0, 0.5ms
  mySensorA.settings.filter = 0; //  0, filter off
  mySensorA.settings.tempOverSample = 1;
  mySensorA.settings.pressOverSample = 1;
  mySensorA.settings.humidOverSample = 1;
  Serial.println("Starting BME280s... result of .begin():");
  delay(10);  //Make sure sensor had enough time to turn on. BME280 requires 2ms to start up.
  Serial.print("Sensor A: 0x");
  Serial.println(mySensorA.begin(), HEX);
}
