#include <Wire.h>
#include <ArduCAM.h>
#include "memorysaver.h"

#include <stdint.h>
#include "SparkFunBME280.h"
#include "Adafruit_AS726x.h"

#include <SD.h>
File myFile;

#include <Adafruit_GPS.h>
#define GPSSerial Serial1
Adafruit_GPS GPS(&GPSSerial);
#define GPSECHO true

#include <SPI.h>
#include <RH_RF95.h>

//for feather m0 RFM9x
#define RFM95_CS 13 //Or take the DIGITAL pin number connected from SX1278 NSS to board
#define RFM95_RST 11 //Or take the DIGITAL pin number connected from SX1278 RST to board
#define RFM95_INT 12 //Or take the DIGITAL pin number connected from SX1278 DIO0 to board


// Blinky on receipt
#define LED 13

#define RF95_FREQ 433.0

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

uint8_t bmedata[16];
uint8_t spectraldata[14];
uint8_t gpsdatetime[9];
uint8_t gpslocation[22];
uint32_t timer = millis(); //Delay timer for GPS
const int sdchipselect = 4; //For SD Card

// set pin 7 as the slave select for the digital pot:
const int CS = 6; //For arducam
bool is_header = false;
int mode = 0;
uint8_t start_capture = 0;
int y=0;
uint8_t datastream[15360];

Adafruit_AS726x ams;
uint16_t sensorValues[AS726x_NUM_CHANNELS];

BME280 mySensorA;

#define VBATPIN A7 //For battery measurements, PIN D9
float battlevel;


uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
uint8_t len = sizeof(buf);
uint8_t bufferx[66];
int loopcount;
float percent;
String comd="";

union u_tag
{
  uint8_t b[4];
  float fval;
}u;

void battcall()
{
  Serial.print("\nReceived ");
  Serial.println(comd);
  battlevel = analogRead(VBATPIN);
  Serial.println(battlevel);
  battlevel *= 2; //Voltage divided by 2
  battlevel *= 3.3; //Reference voltage is 3.3V
  Serial.println(battlevel);
  battlevel /= 1024; //Dk why, prolly scaled, converts it to voltage supposedly
  Serial.println(battlevel);

  u.fval = battlevel;
  uint8_t reply[4];
  reply[0]=u.b[0];
  reply[1]=u.b[1];
  reply[2]=u.b[2];
  reply[3]=u.b[3];
  rf95.send(reply, sizeof(reply));
  delay(10);
  rf95.waitPacketSent();  
}

//This demo can only work on OV2640_MINI_2MP_PLUS platform.
#if !(defined OV2640_MINI_2MP_PLUS)
  #error Please select the hardware platform and camera module in the ../libraries/ArduCAM/memorysaver.h file
#endif

#if defined (OV2640_MINI_2MP_PLUS)
  ArduCAM myCAM( OV2640, CS );
#endif

uint8_t read_fifo_burst(ArduCAM myCAM)
{
  uint8_t temp = 0, temp_last = 0;
  uint32_t length=0;
  length = myCAM.read_fifo_length();
  Serial.print("Image file size is: ");
  uint8_t filelen[4];
  filelen[0]=(uint8_t)(length%256);
  filelen[1]=(uint8_t)(length/256 %256);
  filelen[2]=(uint8_t)(length/256 /256 %256);
  filelen[3]=(uint8_t)(length/256 /256 /256 %256);
  Serial.println(length, DEC);
  rf95.send(filelen, sizeof(filelen));
  rf95.waitPacketSent();
  if (length >= MAX_FIFO_SIZE) //512 kb
  {
    Serial.println(F("ACK CMD Over size. END"));
    return 0;
  }
  if (length == 0 ) //0 kb
  {
    Serial.println(F("ACK CMD Size is 0. END"));
    return 0;
  }
  myCAM.CS_LOW();
  myCAM.set_fifo_burst();//Set fifo burst mode
  temp =  SPI.transfer(0x00);
  y=0;
  datastream[y++]=temp;
  length --;
  while ( length-- )
  {
    temp_last = temp;
    temp =  SPI.transfer(0x00);
    datastream[y++]=temp;
    if (is_header == true)
    {
//      Serial.write(temp);
    }
    else if ((temp == 0xD8) & (temp_last == 0xFF))
    {
      is_header = true;
      Serial.println(F("ACK IMG END"));
    }
    if ( (temp == 0xD9) && (temp_last == 0xFF) ) //If find the end ,break while,
    break;
    delayMicroseconds(15);
  }
  myCAM.CS_HIGH();
  is_header = false;
  return 1;
}

void sdcardsetup()
{ 
  while (!SD.begin(sdchipselect)) {
    Serial.println("Card failed, or not present");
    delay(1000);
    // don't do anything more:
  }
  Serial.println("card initialized.");
}

void gpssetup()
{
  Serial.print("Initializing GPS...");
  
  GPS.begin(9600);
  GPS.sendCommand(PMTK_SET_NMEA_OUTPUT_RMCGGA);
  GPS.sendCommand(PMTK_SET_NMEA_UPDATE_1HZ); // 1 second update time
  GPS.sendCommand(PGCMD_ANTENNA);
  delay(1000);
  GPSSerial.println(PMTK_Q_RELEASE);
}

void gpscall()
{
  char gpsc = GPS.read();
  
/*  if (GPSECHO)
    if (gpsc) Serial.print(gpsc); //For Debugging, testing*/
  if (GPS.newNMEAreceived()) 
  {
    if (!GPS.parse(GPS.lastNMEA()))
    { 
      //Insert code to LoRa to groundstation GPS failed.
      return;
    }
      
  }

  if (timer > millis()) timer = millis();

  
  static unsigned nextInterval = 2000;
  if (millis() - timer > nextInterval) {
    timer = millis(); // reset the timer
    nextInterval = 1500 + random(1000);
    float s = GPS.seconds + GPS.milliseconds/1000. + GPS.secondsSinceTime(); 
    int m = GPS.minute;
    int h = 8+GPS.hour;
    int d = GPS.day;
    while(s > 60){ s -= 60; m++; }    
    while(m > 60){ m -= 60; h++; }
    while(h > 24){ h -= 24; d++; }
    // ISO Standard Date Format, with leading zeros https://xkcd.com/1179/ 
    Serial.print("\nDate: ");   
    Serial.print(GPS.year+2000, DEC); Serial.print("-");
    if(GPS.month < 10) Serial.print("0");
    Serial.print(GPS.month, DEC); Serial.print("-");
    if(d < 10) Serial.print("0");
    Serial.print(d, DEC);
    Serial.print("   Time: ");
    if(h < 10) Serial.print("0");
    Serial.print(h, DEC); Serial.print(':');
    if(m < 10) Serial.print("0");
    Serial.print(m, DEC); Serial.print(':');
    if(s < 10) Serial.print("0");
    Serial.println(s, 3);
    Serial.print("Fix: "); Serial.print((int)GPS.fix);
    Serial.print(" quality: "); Serial.println((int)GPS.fixquality);
    if (GPS.fix) {
      Serial.print("Location: ");
      Serial.print(GPS.latitude, 4); Serial.print(GPS.lat);
      Serial.print(", ");
      Serial.print(GPS.longitude, 4); Serial.println(GPS.lon);
      Serial.print("Speed (knots): "); Serial.println(GPS.speed);
      Serial.print("Angle: "); Serial.println(GPS.angle);
      Serial.print("Altitude: "); Serial.println(GPS.altitude);
      Serial.print("Satellites: "); Serial.println((int)GPS.satellites);
    }
    
    gpsdatetime[0] = d;
    gpsdatetime[1] = GPS.month;
    gpsdatetime[2] = GPS.year;
    gpsdatetime[3] = h;
    gpsdatetime[4] = m;
    
    u.fval = s;
    gpsdatetime[5] = u.b[0];
    gpsdatetime[6] = u.b[1];
    gpsdatetime[7] = u.b[2];
    gpsdatetime[8] = u.b[3];
    
    float lat = GPS.latitude;
    float lon = GPS.longitude;
    float alt = GPS.altitude;
    float ang = GPS.angle;
    float spd = GPS.speed;
    
    u.fval = lat;
    gpslocation[0] = u.b[0];
    gpslocation[1] = u.b[1];
    gpslocation[2] = u.b[2];
    gpslocation[3] = u.b[3];
    gpslocation[4] = (char)GPS.lat;
    
    u.fval = lon;
    gpslocation[5]= u.b[0];
    gpslocation[6]= u.b[1];
    gpslocation[7]= u.b[2];
    gpslocation[8]= u.b[3];
    gpslocation[9]= (char)GPS.lon;

    u.fval = alt;
    gpslocation[10]= u.b[0];
    gpslocation[11]= u.b[1];
    gpslocation[12]= u.b[2];
    gpslocation[13]= u.b[3];

    u.fval = ang;
    gpslocation[14]= u.b[0];
    gpslocation[15]= u.b[1];
    gpslocation[16]= u.b[2];
    gpslocation[17]= u.b[3];
    
    u.fval = spd;
    gpslocation[18]= u.b[0];
    gpslocation[19]= u.b[1];
    gpslocation[20]= u.b[2];
    gpslocation[21]= u.b[3];
  }
}

void sendgps()
{  
  rf95.send(gpsdatetime, sizeof(gpsdatetime));
  rf95.waitPacketSent();
  delay(100);
  rf95.send(gpslocation, sizeof(gpslocation));
  rf95.waitPacketSent();
}

void Arducamsetup()
{
  // put your setup code here, to run once:
  uint8_t vid, pid;
  uint8_t temp;
  Wire.begin();
//  Serial.begin(921600);
  Serial.println(F("ACK CMD ArduCAM Start! END"));
  // set the CS as an output:
  pinMode(CS, OUTPUT);
  digitalWrite(CS, HIGH);
  // initialize SPI:
  SPI.begin();
    //Reset the CPLD
  myCAM.write_reg(0x07, 0x80);
  delay(100);
  myCAM.write_reg(0x07, 0x00);
  delay(100);
  while(1){
    //Check if the ArduCAM SPI bus is OK
    myCAM.write_reg(ARDUCHIP_TEST1, 0x55);
    temp = myCAM.read_reg(ARDUCHIP_TEST1);
    if (temp != 0x55){
      Serial.println(F("ACK CMD SPI interface Error! END"));
      delay(1000);continue;
    }
    else{
      Serial.println(F("ACK CMD SPI interface OK. END"));break;
    }
  }

#if defined (OV2640_MINI_2MP_PLUS)
  while(1){
    //Check if the camera module type is OV2640
    myCAM.wrSensorReg8_8(0xff, 0x01);
    myCAM.rdSensorReg8_8(OV2640_CHIPID_HIGH, &vid);
    myCAM.rdSensorReg8_8(OV2640_CHIPID_LOW, &pid);
    if ((vid != 0x26 ) && (( pid != 0x41 ) || ( pid != 0x42 ))){
      Serial.println(F("ACK CMD Can't find OV2640 module! END"));
      delay(1000);continue;
    }
    else{
      Serial.println(F("ACK CMD OV2640 detected. END"));break;
    } 
  }
#endif
//Change to JPEG capture mode and initialize the OV5642 module
myCAM.set_format(JPEG);
myCAM.InitCAM();
#if defined (OV2640_MINI_2MP_PLUS)
  myCAM.OV2640_set_JPEG_size(OV2640_320x240);
#endif
delay(1000);
myCAM.clear_fifo_flag();
#if !(defined (OV2640_MINI_2MP_PLUS))
myCAM.write_reg(ARDUCHIP_FRAMES,0x00);
#endif
}

void Arducamcall()
{
  digitalWrite(LED, HIGH); //Set LED to on
  //RH_RF95::printBuffer("Received: ", buf, len);
  Serial.print("\nReceieved ");
  Serial.println(comd);

  // Send a reply
  uint8_t reply[] = "Capturing picture";
  rf95.send(reply, sizeof(reply));
  rf95.waitPacketSent();
  Serial.println("Sent a reply");
  digitalWrite(LED, LOW);
  rf95.sleep();
  delay(1000);

  uint8_t temp = 0xff, temp_last = 0;
  bool is_header = false;
/*  myCAM.OV2640_set_JPEG_size(OV2640_320x240);delay(1000);
  Serial.println(F("ACK CMD switch to OV2640_320x240 END"));
  temp = 0xff;*/
  myCAM.OV2640_set_JPEG_size(OV2640_352x288);delay(1000);
  Serial.println(F("ACK CMD switch to OV2640_352x288 END"));
  temp = 0xff;
/*  myCAM.OV2640_set_JPEG_size(OV2640_640x480);delay(1000);
  Serial.println(F("ACK CMD switch to OV2640_640x480 END"));
  temp = 0xff;
  myCAM.OV2640_set_JPEG_size(OV2640_800x600);delay(1000);
  Serial.println(F("ACK CMD switch to OV2640_800x600 END"));
  temp = 0xff;
  myCAM.OV2640_set_JPEG_size(OV2640_1024x768);delay(1000);
  Serial.println(F("ACK CMD switch to OV2640_1024x768 END"));
  temp = 0xff;
  myCAM.OV2640_set_JPEG_size(OV2640_1280x1024);delay(1000);
  Serial.println(F("ACK CMD switch to OV2640_1280x1024 END"));
  temp = 0xff;
  myCAM.OV2640_set_JPEG_size(OV2640_1600x1200);delay(1000);
  Serial.println(F("ACK CMD switch to OV2640_1600x1200 END"));
  temp = 0xff;*/
  mode = 1;
  start_capture = 1;
  Serial.println(F("ACK CMD CAM start single shoot. END"));
/*  myCAM.set_format(JPEG);
  myCAM.InitCAM();*/
  if (mode == 1)
  {
    if (start_capture == 1)
    {
      Serial.print("Capturing...\n");
      myCAM.flush_fifo();
      myCAM.clear_fifo_flag();
      //Start capture
      myCAM.start_capture();
      start_capture = 0;
    }
/*    if (myCAM.get_bit(ARDUCHIP_TRIG, CAP_DONE_MASK))
    {*/
      Serial.println(F("ACK CMD CAM Capture Done. END"));delay(50);
      read_fifo_burst(myCAM);
      //Clear the capture done flag
      myCAM.clear_fifo_flag();
/*    }*/
  }
}



void setup()
{
  pinMode(LED, OUTPUT);
  /*pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);*/

  Serial.begin(921600);
/*  while (!Serial) {  //This is to hold the module if not connected via USB
    delay(1);
  }
  delay(100);*/
/*
  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);
*/
  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    while (1);
  }
  Serial.println("LoRa radio init OK!");

  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }
  Serial.print("Set Freq to(MHz): "); Serial.println(RF95_FREQ);

  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on

  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(23, false);

  Arducamsetup();
  gpssetup();
//  sdcardsetup();
  bme280setup();
  spectralsetup();
}

void pong() //Function to reply a ping call
{
  digitalWrite(LED, HIGH); //Set LED to on
  //RH_RF95::printBuffer("Received: ", buf, len);
  Serial.print("Receieved ");
  Serial.println(comd);
  Serial.print("RSSI: ");
  Serial.println(rf95.lastRssi(), DEC);

  // Send a reply
  uint8_t reply[] = "Pong";
  rf95.send(reply, sizeof(reply));
  rf95.waitPacketSent();
  Serial.println("Sent a reply");
  digitalWrite(LED, LOW);
}

void datasend() //Sends simple jpg picture in base64, replace with function for other data
{
  uint8_t data[y];
  for(int x=0;x<y;x++)
  {
    data[x]=datastream[x];
  }
  digitalWrite(LED, HIGH); //Set LED to on
  Serial.print("Sending JPG payload.");
  for (loopcount=0;(loopcount*4096)<=y;loopcount++)
  {
    Serial.print("\n");
    Serial.print("Transmitting: ");
    percent = ((float)(loopcount*409600)/((float)y));
    Serial.print(percent);
    Serial.print("%");
    Serial.print("\n");
    for (int i=0;i<64;i++)
    {
      if(((loopcount*4096)+(i*64))>=y)
      {
        delay(500);
        break;
      }
      for (int r=0;r<64;r++)
      {
        if (((loopcount*4096)+(i*64)+r)<y)
        {
          bufferx[r]=data[(loopcount*4096)+i*64+r];
          if (bufferx[r]<16)
          {
            Serial.print(" 0x0");
            Serial.print(bufferx[r], HEX);
          }
          else
          {
            Serial.print(" 0x");
            Serial.print(bufferx[r], HEX);
          }
        }
        else
        {
          bufferx[r]=0;
          Serial.print(" 0x0");
          Serial.print(bufferx[r], HEX);
        }
      }
      rf95.send(bufferx, sizeof(bufferx));
      delay(100);
      rf95.waitPacketSent();
      delay(900);
    }
  }
  uint8_t endx[] = "End transmission";
  rf95.send(endx, sizeof(endx));
  rf95.waitPacketSent();
  Serial.println("\nSent JPG payload.");
  digitalWrite(LED, LOW);
}

void wrongcommand() //Replies to an unknown command
{
  digitalWrite(LED, HIGH); //Set LED to on
  //RH_RF95::printBuffer("Received: ", buf, len);
  Serial.print("Receieved: ");
  Serial.println(comd);
  Serial.println("Unknown command. Replying...");
  uint8_t reply[] = "Unknown Command";
  rf95.send(reply, sizeof(reply));
  rf95.waitPacketSent();
  Serial.println("Sent a reply");
  digitalWrite(LED, LOW);
}

void readlora() //Function to call LoRa
{
  if (rf95.available())
  {
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);

    if (rf95.recv(buf, &len)) //Check what message receieved
    {      
      comd = (char*)buf;
      if(strcmp((char*)buf,"Ping") == 0) //Pingpong check
      {
        pong();
      }
      else if(strcmp((char*)buf,"Download") == 0) //Sending data command
      {
        datasend();
      }
      else if(strcmp((char*)buf,"Capture") == 0)
      {
        Arducamcall();
        delay(1000);
        rf95.setModeIdle();
      }
      else if(strcmp((char*)buf,"GPS") == 0)
      {
        sendgps();
        delay(50);
      }
      else if(strcmp((char*)buf,"BME280") == 0)
      {
        bme280call();
        delay(50);
      }
      else if(strcmp((char*)buf,"Spectral") == 0)
      {
        spectralcall();
        delay(50);
      }
      else if(strcmp((char*)buf,"Sitrep") == 0)
      {
        battcall();
        delay(10);
      }
      else
      {
        wrongcommand();
      }
    }
    else
    {
      Serial.println("Receive failed");
    }
  }
}

void loop(){
  readlora();
  gpscall();
}
