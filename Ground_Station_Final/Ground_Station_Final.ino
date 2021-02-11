
#include <SPI.h>
#include <RH_RF95.h>

//for feather m0  
#define RFM95_CS 8 //Or take the DIGITAL pin number connected from SX1278 NSS to board
#define RFM95_RST 4 //Or take the DIGITAL pin number connected from SX1278 RST to board
#define RFM95_INT 3 //Or take the DIGITAL pin number connected from SX1278 DIO0 to board


/* for shield 
#define RFM95_CS 10
#define RFM95_RST 9
#define RFM95_INT 7
*/

/* Feather m0 w/wing 
#define RFM95_RST     11   // "A"
#define RFM95_CS      10   // "B"
#define RFM95_INT     6    // "D"
*/

#define RF95_FREQ 433.0

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);

// Blinky on receipt
#define LED 13

int is_receiving = 0;
int i=0, j=0;
char rx_b = 0;
String rx_s="";
uint8_t stream[20480];
int16_t packetnum = 0;  // packet counter, we increment per xmission
uint32_t filesize;

union h_tag
{
  uint8_t b2[2];
  uint16_t halfb;
}h;

union u_tag
{
  uint8_t b[4];
  float fval;
}u;



void loranext()
{
  // Now wait for a reply
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);

  if (rf95.waitAvailableTimeout(1000))
  { 
    // Should be a reply message for us now   
    if (rf95.recv(buf, &len))
    {
      digitalWrite(LED, HIGH);
      Serial.println((char*)buf);
      Serial.print("RSSI: ");
      Serial.println(rf95.lastRssi(), DEC);
      digitalWrite(LED, LOW);    
    }
    else
    {
      Serial.println("Receive failed");
    }
  }
  else
  {
    Serial.println("No reply, is there a listener around?");
  }
}

void readcomd()
{
  if (Serial.available() && is_receiving == 0) // Check if character exists
  {
    rx_b = Serial.read(); // Get the Character
    if(rx_b != '\n') // A character of the string is received except newline
    {
      rx_s += rx_b;
    }
    else
    {
      checkcomd();
    }
  }
  else rx_s="";
}

void sitrep()
{
  digitalWrite(LED, HIGH);
  uint8_t comd[] = "Sitrep";
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
  if (rf95.waitAvailableTimeout(1000))
  { 
    // Should be a reply message for us now   
    if (rf95.recv(buf, &len))
    {
      digitalWrite(LED, HIGH);
      u.b[0]=buf[0];
      u.b[1]=buf[1];
      u.b[2]=buf[2];
      u.b[3]=buf[3];
      Serial.print("Battery level: ");
      Serial.print(u.fval);
      Serial.print("V ");
      Serial.print("RSSI: ");
      Serial.println(rf95.lastRssi(), DEC);
      digitalWrite(LED, LOW);    
    }
    else
    {
      Serial.println("Receive failed");
    }
  }
  else
  {
    Serial.println("Unable to receive housekeeping data.");
  }
}

void setup() 
{
/*  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);*/

  Serial.begin(921600);
  while (!Serial) {
    delay(1);
  }

  delay(100);

  Serial.println("Ground Station Setup.");
/*
  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);
*/
  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    Serial.println("Uncomment '#define SERIAL_DEBUG' in RH_RF95.cpp for detailed debug info");
    while (1);
  }
  Serial.println("LoRa radio init OK!");

  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
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

}

void checkcomd()
{
  if (rx_s == "ping")
  {
    digitalWrite(LED, HIGH);
    uint8_t comd[] = "Ping";
    Serial.println("Transmitting..."); // Send a message to rf95_server
    //  itoa(packetnum++, comd+13, 10);
    Serial.print("Sending "); Serial.println((char*)comd);
  
    delay(10);
    rf95.send(comd, sizeof(comd));

    Serial.println("Waiting for packet to complete..."); 
    delay(10);
    rf95.waitPacketSent();
    rx_s = "";
    digitalWrite(LED, LOW);
  
    // Now wait for a reply
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);

    Serial.println("Waiting for reply...");
    if (rf95.waitAvailableTimeout(1000))
    { 
      // Should be a reply message for us now   
      if (rf95.recv(buf, &len))
      {
        digitalWrite(LED, HIGH);
        Serial.print("Got reply: ");
        Serial.println((char*)buf);
        Serial.print("RSSI: ");
        Serial.println(rf95.lastRssi(), DEC);
        digitalWrite(LED, LOW);    
      }
      else
      {
        Serial.println("Receive failed");
      }
    }
    else
    {
      Serial.println("No reply, is there a listener around?");
    }
  }
  else if (rx_s == "clear")
  {
    stream[25600] = {0};
    i=0;
    Serial.print("Stream memory and counter cleared.\n");
    rx_s = "";
  }
  else if (rx_s == "save")
  {
    for (j=0;j<(i+1);j++)
    {
      if (stream[j]<16)
      {
        Serial.print(" 0x0");
        Serial.print(stream[j], HEX);
      }
      else
      {
        Serial.print(" 0x");
        Serial.print(stream[j], HEX);
      }
    }
//    Serial.write(stream, (i+1));
    Serial.print("\nStream written to serial and saved.\n");
    rx_s = "";
  }
  else if (rx_s == "download")
  {
    digitalWrite(LED, HIGH);
    uint8_t comd[] = "Download";
    Serial.println("Transmitting..."); // Send a message to rf95_server
    //  itoa(packetnum++, comd+13, 10);
    Serial.print("Sending "); Serial.println((char*)comd);
  
    delay(10);
    rf95.send(comd, sizeof(comd));

    Serial.println("Waiting for data download to complete..."); 
    delay(10);
    rf95.waitPacketSent();
    rx_s = "";
    digitalWrite(LED, LOW);
  }
  else if (rx_s == "sitrep")
  {
    sitrep();
  }
  else if (rx_s == "bme280")
  {
    bme280();
  }
  else if (rx_s == "spectral")
  {
    spectral();
  }
  else if (rx_s == "gps")
  {
    digitalWrite(LED, HIGH);
    uint8_t comd[] = "GPS";
    Serial.println("Transmitting..."); // Send a message to rf95_server
    Serial.print("Sending "); Serial.println((char*)comd);
  
    delay(10);
    rf95.send(comd, sizeof(comd));

    Serial.println("Waiting for command to complete..."); 
    delay(10);
    rf95.waitPacketSent();
    rx_s = "";
    digitalWrite(LED, LOW);

    // Now wait for a reply
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    
    if (rf95.waitAvailableTimeout(1000))
    { 
      // Should be a reply message for us now   
      if (rf95.recv(buf, &len))
      {
        digitalWrite(LED, HIGH);
        Serial.print("Date: ");
        if(buf[0] < 10) Serial.print("0");
        Serial.print(buf[0]);
        Serial.print("-");
        if(buf[1] < 10) Serial.print("0");
        Serial.print(buf[1]);
        Serial.print("-");
        Serial.print(buf[2]+2000);
        Serial.print(" Time: ");
        if(buf[3] < 10) Serial.print("0");
        Serial.print(buf[3]);
        Serial.print(":");
        if(buf[4] < 10) Serial.print("0");
        Serial.print(buf[4]);
        Serial.print(":");
        u.b[0]=buf[5];
        u.b[1]=buf[6];
        u.b[2]=buf[7];
        u.b[3]=buf[8];
        if(u.fval < 10) Serial.print("0");
        Serial.print(u.fval);
        Serial.print(" RSSI: ");
        Serial.println(rf95.lastRssi(), DEC);
        digitalWrite(LED, LOW);    
      }
      else
      {
        Serial.println("Receive failed");
      }
    }
    
    // Now wait for a reply
    uint8_t buf2[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len2 = sizeof(buf);
    
    if (rf95.waitAvailableTimeout(1000))
    { 
      // Should be a reply message for us now   
      if (rf95.recv(buf2, &len2))
      {
        digitalWrite(LED, HIGH);
        Serial.print("Lat: ");
        u.b[0]=buf2[0];
        u.b[1]=buf2[1];
        u.b[2]=buf2[2];
        u.b[3]=buf2[3];
        Serial.print(u.fval);
        Serial.print((char)buf2[4]);
        
        Serial.print(" Long: ");
        u.b[0]=buf2[5];
        u.b[1]=buf2[6];
        u.b[2]=buf2[7];
        u.b[3]=buf2[8];
        Serial.print(u.fval);
        Serial.print((char)buf2[9]);
        
        Serial.print(" Alt: ");
        u.b[0]=buf2[10];
        u.b[1]=buf2[11];
        u.b[2]=buf2[12];
        u.b[3]=buf2[13];
        Serial.print(u.fval);
        
        Serial.print(" Angle: ");
        u.b[0]=buf2[14];
        u.b[1]=buf2[15];
        u.b[2]=buf2[16];
        u.b[3]=buf2[17];
        Serial.print(u.fval);
        
        Serial.print(" Speed(knots): ");
        u.b[0]=buf2[18];
        u.b[1]=buf2[19];
        u.b[2]=buf2[20];
        u.b[3]=buf2[21];
        Serial.print(u.fval);
        Serial.print(" RSSI: ");
        Serial.println(rf95.lastRssi(), DEC);
        digitalWrite(LED, LOW);    
      }
      else
      {
        Serial.println("Receive failed");
      }
    }
    else
    {
      Serial.println("CubeSat not sending GPS data.");
    }
  }
  else if (rx_s == "capture")
  {
    digitalWrite(LED, HIGH);
    uint8_t comd[] = "Capture";
    Serial.println("Transmitting..."); // Send a message to rf95_server
    //  itoa(packetnum++, comd+13, 10);
    Serial.print("Sending "); Serial.println((char*)comd);
  
    delay(10);
    rf95.send(comd, sizeof(comd));

    Serial.println("Waiting for data download to complete..."); 
    delay(10);
    rf95.waitPacketSent();
    rx_s = "";
    digitalWrite(LED, LOW);
    
    // Now wait for a reply
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);

    Serial.println("Waiting for reply...");
    if (rf95.waitAvailableTimeout(5000))
    { 
      // Should be a reply message for us now   
      if (rf95.recv(buf, &len))
      {
        digitalWrite(LED, HIGH);
        Serial.print("Got reply: ");
        Serial.println((char*)buf);
        Serial.print("RSSI: ");
        Serial.println(rf95.lastRssi(), DEC);
        digitalWrite(LED, LOW);    
      }
      else
      {
        Serial.println("Receive failed");
      }
    }
    delay(100);
    uint8_t buf2[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len2 = sizeof(buf2);

    while(!rf95.available())
    {
      if (rf95.waitAvailableTimeout(5000));
      else
      {
        Serial.print("Cubesat capture failed.");
        break;
      }
    }
    if (rf95.recv(buf2, &len2))
    {
      filesize =(uint32_t)buf2[0]+(uint32_t)buf2[1]*256+(uint32_t)buf2[2]*256*256+(uint32_t)buf2[3]*256*256*256;
      digitalWrite(LED, HIGH);
      Serial.print("File size in bytes: ");
      Serial.println(filesize, DEC);
      Serial.print("RSSI: ");
      Serial.println(rf95.lastRssi(), DEC);
      digitalWrite(LED, LOW);    
    }
    else
    {
      Serial.print("Jpeg file size receive failed.\n");
    }
  }
  else if (rx_s == "knock")
  {
    digitalWrite(LED, HIGH);
    uint8_t comd[] = "Knock";
    Serial.println("Transmitting..."); // Send a message to rf95_server
    //  itoa(packetnum++, comd+13, 10);
    Serial.print("Sending "); Serial.println((char*)comd);
  
    delay(10);
    rf95.send(comd, sizeof(comd));

    Serial.println("Waiting for packet to complete..."); 
    delay(10);
    rf95.waitPacketSent();
    rx_s = "";
    digitalWrite(LED, LOW);
    
    // Now wait for a reply
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);

    Serial.println("Waiting for reply...");
    if (rf95.waitAvailableTimeout(1000))
    { 
      // Should be a reply message for us now   
      if (rf95.recv(buf, &len))
      {
        digitalWrite(LED, HIGH);
        Serial.print("Got reply: ");
        Serial.println((char*)buf);
        Serial.print("RSSI: ");
        Serial.println(rf95.lastRssi(), DEC);
        digitalWrite(LED, LOW);    
      }
      else
      {
        Serial.println("Receive failed");
      }
    }
  }
  else
  {
    Serial.print("Unknown command.\n");
    rx_s = "";
  }
}

void listening()
{
  if (rf95.available())
  {
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);
    if (rf95.recv(buf, &len)) //Check what message receieved
    {
      digitalWrite(LED, HIGH);
      if (is_receiving == 0)
      {
        Serial.print("Downloading... Please wait.\n");
        is_receiving = 1;
      }
      if (strcmp((char*)buf,"End transmission") == 0)
      {
        Serial.print("\nDownload RSSI: ");
        Serial.println(rf95.lastRssi(), DEC);
        Serial.print("End of data transmission.\n");
        is_receiving = 0;
        digitalWrite(LED, LOW);
      }
      else
      {
        for (j=0;j<64;j++)
        {
          if (i>=20480)
          {
            Serial.print("Stream space full.");
            i=0;
            return;
          }
          else
          {
            stream[i]=buf[j];
            i++;
          }
        }
      }
    }
    else
    {
      Serial.println("Data incoming failed.");
    }
  }
}

void loop()
{  
  readcomd();
  if(Serial.available()<=0)
  {
    Serial.print("Please input command or await incoming transmission.\n");
    while(Serial.available()<=0)
    {
      listening();
    }
  }
}
