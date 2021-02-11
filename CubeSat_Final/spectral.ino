void spectralsetup()
{
  while(!ams.begin()){
    Serial.println("could not connect to sensor! Please check your wiring.");
    delay(1000);
  }
}

void spectralcall()
{
  union h_tag
  {
    uint8_t b2[2];
    uint16_t halfb;
  }h;
  
  //read the device temperature
  uint16_t temp = ams.readTemperature();
  uint16_t violet,blue,green,yellow,orange,red;
  
  //ams.drvOn(); //uncomment this if you want to use the driver LED for readings
  ams.startMeasurement(); //begin a measurement
  
  //wait till data is available
  bool rdy = false;
  while(!rdy){
    delay(5);
    rdy = ams.dataReady();
  }
  //ams.drvOff(); //uncomment this if you want to use the driver LED for readings

  //read the values!
  ams.readRawValues(sensorValues);
  //ams.readCalibratedValues(calibratedValues);

  h.halfb=temp;
  spectraldata[0]=h.b2[0];
  spectraldata[1]=h.b2[1];

  violet=sensorValues[AS726x_VIOLET];
  h.halfb=violet;
  spectraldata[2]=h.b2[0];
  spectraldata[3]=h.b2[1];
  
  blue=sensorValues[AS726x_BLUE];
  h.halfb=blue;
  spectraldata[4]=h.b2[0];
  spectraldata[5]=h.b2[1];
  
  green=sensorValues[AS726x_GREEN];
  h.halfb=green;
  spectraldata[6]=h.b2[0];
  spectraldata[7]=h.b2[1];
  
  yellow=sensorValues[AS726x_YELLOW];
  h.halfb=yellow;
  spectraldata[8]=h.b2[0];
  spectraldata[9]=h.b2[1];
  
  orange=sensorValues[AS726x_ORANGE];
  h.halfb=orange;
  spectraldata[10]=h.b2[0];
  spectraldata[11]=h.b2[1];
  
  red=sensorValues[AS726x_RED];
  h.halfb=red;
  spectraldata[12]=h.b2[0];
  spectraldata[13]=h.b2[1];
  
  rf95.send(spectraldata, sizeof(spectraldata));
  rf95.waitPacketSent();
  delay(100);

  myFile = SD.open("spectral.txt", FILE_WRITE);

  // if the file opened okay, write to it:
  if (myFile) {
    Serial.print("Writing to SD...");
    myFile.println("Spectral Sensor Data:");
    myFile.print("Temp: "); myFile.print(temp); 
    myFile.print(" Violet: "); myFile.print(sensorValues[AS726x_VIOLET]);
    myFile.print(" Blue: "); myFile.print(sensorValues[AS726x_BLUE]);
    myFile.print(" Green: "); myFile.print(sensorValues[AS726x_GREEN]);
    myFile.print(" Yellow: "); myFile.print(sensorValues[AS726x_YELLOW]);
    myFile.print(" Orange: "); myFile.print(sensorValues[AS726x_ORANGE]);
    myFile.print(" Red: "); myFile.print(sensorValues[AS726x_RED]);
    myFile.println();
    myFile.close();
    Serial.println("done.");
  } 
  else {
    // if the file didn't open, print an error:
    Serial.println("error opening spectral.txt");
  }
}
