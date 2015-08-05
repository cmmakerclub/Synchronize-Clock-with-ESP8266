#define time_zone 3600*7

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <WiFiHelper.h>

// OLED  
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <ESP_Adafruit_SSD1306.h>

#define OLED_RESET 4
Adafruit_SSD1306 display(OLED_RESET);
#define NUMFLAKES 10
#define XPOS 0
#define YPOS 1
#define DELTAY 2
//

const char* ssid     = "MAKERCLUB-CM";
const char* pass     = "welcomegogogo";


unsigned int localPort = 2390;      // local port to listen for UDP packets

IPAddress timeServer(129, 6, 15, 28); // time.nist.gov NTP server

const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message

byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets

// A UDP instance to let us send and receive packets over UDP
WiFiUDP udp;

WiFiHelper wifi(ssid, pass);


void init_wifi()
{
    wifi.on_connecting([](const void* message)
    {
        Serial.println ("connecting..");
    });

    wifi.on_connected([](const void* message)
    {
        Serial.println ((char*)message);
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());     

        //
        Serial.println("Starting UDP");
        udp.begin(localPort);
        Serial.print("Local port: ");
        Serial.println(udp.localPort());
        //   
    });

    wifi.begin();
}

void init_hardware()
{
    Serial.begin(115200);
    delay(10);
    Serial.println();
    Serial.println();


    // OLED 

    display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
    delay(100);
    display.clearDisplay();
    //
}

void setup()
{
    init_hardware();
    init_wifi();
}

void loop()
{
    wifi.loop();
    NTP_get();
}

void NTP_get(void)
{
  sendNTPpacket(timeServer); // send an NTP packet to a time server
  // wait to see if a reply is available
  delay(1000);
  
  int cb = udp.parsePacket();
  if (!cb) {
    Serial.println("no packet yet");
  }
  else {
    Serial.print("packet received, length=");
    Serial.println(cb);
    // We've received a packet, read the data from it
    udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

    //the timestamp starts at byte 40 of the received packet and is four bytes,
    // or two words, long. First, esxtract the two words:

    unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
    unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
    // combine the four bytes (two words) into a long integer
    // this is NTP time (seconds since Jan 1 1900):
    unsigned long secsSince1900 = highWord << 16 | lowWord;
    Serial.print("Seconds since Jan 1 1900 = " );
    Serial.println(secsSince1900);

    // now convert NTP time into everyday time:
    Serial.print("Unix time = ");
    // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
    const unsigned long seventyYears = 2208988800UL;
    // subtract seventy years:
    unsigned long epoch = secsSince1900 - seventyYears;
    // print Unix time:
    Serial.println(epoch);


    // printer to OLED 
        display.clearDisplay();
        display.setTextSize(2);
        display.setTextColor(WHITE);
        display.setCursor(10,10);
        display.clearDisplay();
    //


    // print the hour, minute and second:
    Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)
    //
        display.print(" Time is");
    //
    Serial.print(((epoch + time_zone)  % 86400L) / 3600); // print the hour (86400 equals secs per day)
    //
        display.setCursor(20,35);
        display.print(((epoch + time_zone)  % 86400L) / 3600);
    //
    Serial.print(':');
    //
        display.print(":");
    //
    if ( ((epoch % 3600) / 60) < 10 ) {
      // In the first 10 minutes of each hour, we'll want a leading '0'
      Serial.print('0');
      //
            display.print("0");
      //
    }
    Serial.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)
    //
        display.print((epoch  % 3600) / 60);
    //
    Serial.print(':');
    //
        display.print(":");
    //
    if ( (epoch % 60) < 10 ) {
      // In the first 10 seconds of each minute, we'll want a leading '0'
      Serial.print('0');
      //
            display.print("0");
      //
    }
    Serial.println(epoch % 60); // print the second
    //
        display.print(epoch % 60);
    //
  }
  // wait ten seconds before asking for the time again

  //
      display.display();
  //

  delay(100);
}

// send an NTP request to the time server at the given address
unsigned long sendNTPpacket(IPAddress& address)
{
  Serial.println("sending NTP packet...");
  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
}