#define time_zone 3600*7

//Pin connected to ST_CP of 74HC595
#define latchPin   4
//Pin connected to SH_CP of 74HC595
#define ClK_spi  14
////Pin connected to DS of 74HC595
#define DS   13
////Pin connected to dot led
#define dot   12

#include <ESP8266WiFi.h>
#include <WiFiUdp.h>
#include <WiFiHelper.h>
#include <Ticker.h>

int LED_SEG_TAB[] = {0xfc, 0xc0, 0x6e, 0xe6, 0xd2, 0xb6, 0xbe, 0xe0, 0xfe, 0xf6, 0x00};
//                     0    ------------------------------------------       9 , none
byte dot_state = 0;
unsigned long epoch ;
int hh;
int mm;
int ss;
int force_update= 1;
Ticker second_tick;

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
        // Serial.println ((char*)message);
        // Serial.println("IP address: ");
        // Serial.println(WiFi.localIP());

        // //
        Serial.println("Starting UDP");

        udp.begin(localPort);

        // Serial.print("Local port: ");
        // Serial.println(udp.localPort());
        //
    });

    wifi.begin();
    // while (WiFi.status() != WL_CONNECTED)
    // {
    //     delay(100);
    // }
    // udp.begin(localPort);


}

void init_hardware()
{
    Serial.begin(115200);
    delay(10);
    Serial.println();
    Serial.println();
    //set pins to output so you can control the shift register
    pinMode(latchPin, OUTPUT);
    pinMode(ClK_spi, OUTPUT);
    pinMode(DS, OUTPUT);
    pinMode(dot, OUTPUT);


}

void setup()
{
    init_hardware();
    init_wifi();

    NTP_get();
    delay(100);

    hh = (epoch % 86400L) / 3600;
    mm = (epoch % 3600) / 60;
    ss = (epoch % 60);

    second_tick.attach(1, tick);
}

void loop()
{
    wifi.loop();
    NTP_get();
    delay(1000);
    if (force_update == 0)
    {
        delay(28000);
    }

}

void tick (void)
{
    epoch++; //Add a second
    dot_state = 1 - dot_state;
    digitalWrite(dot, dot_state);

    hh = (epoch % 86400L) / 3600;
    mm = (epoch % 3600) / 60;
    ss = (epoch % 60);

    int hh1 = hh / 10;
    int hh0 = hh % 10;
    if (hh1 == 0) hh1 = 11;

    int mm1 = mm / 10;
    int mm0 = mm % 10;

    digitalWrite(latchPin, LOW);
    shiftOut(DS, ClK_spi, LSBFIRST, LED_SEG_TAB[mm0]);  // digi 0
    shiftOut(DS, ClK_spi, LSBFIRST, LED_SEG_TAB[mm1]);  // digi 1
    shiftOut(DS, ClK_spi, LSBFIRST, LED_SEG_TAB[hh0]);  // digi 2
    shiftOut(DS, ClK_spi, LSBFIRST, LED_SEG_TAB[hh1]);  // digi 3
    digitalWrite(latchPin, HIGH);


}


void NTP_get(void)
{
    sendNTPpacket(timeServer); // send an NTP packet to a time server
    // wait to see if a reply is available
    delay(1000);

    int cb = udp.parsePacket();
    if (!cb) {
        force_update = 1;
    }
    else {
        force_update = 0;
        // Serial.print("packet received, length=");
        // Serial.println(cb);
        // We've received a packet, read the data from it
        udp.read(packetBuffer, NTP_PACKET_SIZE); // read the packet into the buffer

        //the timestamp starts at byte 40 of the received packet and is four bytes,
        // or two words, long. First, esxtract the two words:

        unsigned long highWord = word(packetBuffer[40], packetBuffer[41]);
        unsigned long lowWord = word(packetBuffer[42], packetBuffer[43]);
        // combine the four bytes (two words) into a long integer
        // this is NTP time (seconds since Jan 1 1900):
        unsigned long secsSince1900 = highWord << 16 | lowWord;
        // Serial.print("Seconds since Jan 1 1900 = " );
        // Serial.println(secsSince1900);

        // now convert NTP time into everyday time:
        // Serial.print("Unix time = ");
        // Unix time starts on Jan 1 1970. In seconds, that's 2208988800:
        const unsigned long seventyYears = 2208988800UL;
        // subtract seventy years:
        epoch = secsSince1900 - seventyYears + time_zone;
        // // print Unix time:
        // Serial.println(epoch);

        // // print the hour, minute and second:
        // Serial.print("The UTC time is ");       // UTC is the time at Greenwich Meridian (GMT)

        // Serial.print(((epoch + time_zone)  % 86400L) / 3600); // print the hour (86400 equals secs per day)

        // Serial.print(':');

        // if ( ((epoch % 3600) / 60) < 10 ) {
        //   // In the first 10 minutes of each hour, we'll want a leading '0'
        //   Serial.print('0');

        // }
        // Serial.print((epoch  % 3600) / 60); // print the minute (3600 equals secs per minute)

        // Serial.print(':');

        // if ( (epoch % 60) < 10 ) {
        //   // In the first 10 seconds of each minute, we'll want a leading '0'
        //   Serial.print('0');

        // }
        // Serial.println(epoch % 60); // print the second

    }
    // wait ten seconds before asking for the time again
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