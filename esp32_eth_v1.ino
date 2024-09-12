//////////////////////////////////////////////////////////////////////////////////////////
//ESP32 Environmental Sensor Web Server                                                 //
//                                                                                      //
//Set board to ESP32-Dev Module                                                         //
//I am using a ESP-WROOM-32 module with 30 pins                                         //
//connected to a Wiz W5500 ethernet module                                              //
//and a BME280 (3.3v) environmental sesnor                                              //
//                                                                                      //
//Code is based on the webserver example that comes with the W5500 library              //
//                                                                                      //
//Compiled by Reiley McKerracher                                                        //
//April 2, 2023                                                                         //
//////////////////////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////////////////////
// Initialize                                                                           //
//////////////////////////////////////////////////////////////////////////////////////////

//rename serial, helpful when using multiple serial connections
#define MySerial Serial

//wifi library, only used to turn it off since we are using an ethernet adapter
//library is included esp32 board manager package by Expressif Systems
#include <WiFi.h>

//Ethernet Variable Setup
//install ethernet library from library manager within the arduino IDE
//within the library, copy the Ethernet.h file (within the same directory) to a new name of EthernetESP32.h
//Edit EthernetESP32.h and on line 254 replace "class EthernetServer : public Server" with "class EthernetServer : public Print"
//without the edit, you will have compiling errors for the class
//ethernet code based on web server example that comes with the library
#include <SPI.h>
#include <EthernetESP32.h>  //ethernet library for w5500 chip modified for the esp32
//esp library ETH does not work with the w5500
byte mac[] = { 0x74, 0x69, 0x69, 0x2D, 0x32, 0x33 };  // ethernet mac address - must be unique on your network
IPAddress ip(192, 168, 43, 251);                      // assign an IP address for the server - must be unique on your network and ideally outside your router's dhcp range
IPAddress gateway(192, 168, 43, 106);
IPAddress subnet(255, 255, 255, 0);

EthernetServer server(80);  //set HTTP server port
String readString;          //var for url requested, this allows us to serve different pages based on the url
#define API_RFID "http://192.168.43.251/ProyekKupon/api/rfid/select"


//To create a favicon, first create a 16x16 png file, then convert to base64 then to 8 bit signed hex
//http://www.xiconeditor.com/   -> create and or edit ico file
//https://www.aconvert.com/image/ico-to-png/  -> convert ico to png
//https://www.base64-image.de/    -> png to base64
//https://cryptii.com/pipes/base32-to-hex   -> hex encode
//      text -> decode 64 -> encode 8 bit hexidecimal signed -> text    -> signed 8 bit for ethercard library
//http://tomeko.net/online_tools/base64.php?lang=en    -> base64 to unsigned hex with 0x prefix for ethernet2 library
//for this example I have created a arduino green house/home icon
//code was originally inspired by this forum post: https://jeelabs.org/forum/node/1042.html which unforntatly no longer exists.
const char favicon[] = {
  -0x77, 0x50, 0x4e, 0x47, 0x0d, 0xa, 0x1a, 0xa, 0x00, 0x00, 0x00, 0x0d, 0x49, 0x48, 0x44, 0x52,
  0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x10, 0x2, 0x3, 0x00, 0x00, 0x00, 0x62, -0x63, 0x17,
  -0x0e, 0x00, 0x00, 0x00, 0x04, 0x67, 0x41, 0x4d, 0x41, 0x00, 0x00, -0x4f, -0x71, 0x0b, -0x04, 0x61,
  0x05, 0x00, 0x00, 0x00, 0x20, 0x63, 0x48, 0x52, 0x4d, 0x00, 0x00, 0x7a, 0x26, 0x00, 0x00, -0x80,
  -0x7c, 0x00, 0x00, -0x06, 0x00, 0x00, 0x00, -0x80, -0x18, 0x00, 0x00, 0x75, 0x30, 0x00, 0x00, -0x16,
  0x60, 0x00, 0x00, 0x3a, -0x68, 0x00, 0x00, 0x17, 0x70, -0x64, -0x46, 0x51, 0x3c, 0x00, 0x00, 0x00,
  0x09, 0x50, 0x4c, 0x54, 0x45, 0x00, 0x00, 0x00, 0x00, -0x6a, -0x64, -0x01, -0x01, -0x01, 0x2a, 0x0d,
  0x43, 0x7e, 0x00, 0x00, 0x00, 0x01, 0x74, 0x52, 0x4e, 0x53, 0x00, 0x40, -0x1a, -0x28, 0x66, 0x00,
  0x00, 0x00, 0x01, 0x62, 0x4b, 0x47, 0x44, 0x02, 0x66, 0x0b, 0x7c, 0x64, 0x00, 0x00, 0x00, 0x07,
  0x74, 0x49, 0x4d, 0x45, 0x07, -0x1e, 0x06, 0x10, 0x09, 0x13, 0x0d, 0x58, -0x72, -0x4c, -0x4a, 0x00,
  0x00, 0x00, 0x34, 0x49, 0x44, 0x41, 0x54, 0x08, -0x29, 0x63, 0x60, 0x0c, 0x0d, 0x75, 0x60, 0x10,
  0x0d, 0x0d, 0x0d, -0x7f, 0x10, -0x5f, 0x61, 0x53, -0x7f, 0x44, -0x2c, -0x2e, 0x50, -0x7a, -0x30, -0x54,
  -0x6b, 0x40, 0x62, -0x2b, -0x56, 0x50, -0x7a, -0x50, 0x55, -0x55, -0x5a, 0x42, 0x58, 0x50, 0x22, 0x0a,
  -0x67, 0x40, -0x18, 0x05, 0x19, 0x05, 0x00, -0x35, -0x6e, 0x18, 0x60, 0x40, 0x44, 0x39, -0x49, 0x00,
  0x00, 0x00, 0x25, 0x74, 0x45, 0x58, 0x74, 0x64, 0x61, 0x74, 0x65, 0x3a, 0x63, 0x72, 0x65, 0x61,
  0x74, 0x65, 0x00, 0x32, 0x30, 0x31, 0x38, 0x2d, 0x30, 0x36, 0x2d, 0x31, 0x36, 0x54, 0x30, 0x39,
  0x3a, 0x31, 0x39, 0x3a, 0x31, 0x33, 0x2d, 0x30, 0x37, 0x3a, 0x30, 0x30, -0x7a, 0x13, -0x11, 0x3e,
  0x00, 0x00, 0x00, 0x25, 0x74, 0x45, 0x58, 0x74, 0x64, 0x61, 0x74, 0x65, 0x3a, 0x6d, 0x6f, 0x64,
  0x69, 0x66, 0x79, 0x00, 0x32, 0x30, 0x31, 0x38, 0x2d, 0x30, 0x36, 0x2d, 0x31, 0x36, 0x54, 0x30,
  0x39, 0x3a, 0x31, 0x39, 0x3a, 0x31, 0x33, 0x2d, 0x30, 0x37, 0x3a, 0x30, 0x30, -0x9, 0x4e, 0x57,
  -0x7e, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, -0x52, 0x42, 0x60, -0x7e
};

//BME280 Variable Setup
//Adafruit BME280 Library
//https://github.com/adafruit/Adafruit_BME280_Library/tree/master
#include <Wire.h>
#include <SPI.h>

//Global Variables for time stracking reading of sensors
unsigned long lastSensorTime = millis();
bool berhasil = false;
//////////////////////////////////////////////////////////////////////////////////////////
// Start Functions                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////

/*
W5500 - ESP32
5V - No Connection (NC)
GND - GND PSU (not the ESP32)
RST - NC
INT - NC
GND - NC
3.3V - 3.3V PSU (not the ESP32)
MISO - GPIO 19
MOSI - GPIO 23
SCS - GPIO 5
SCLK - GPIO 18
*/

//return how long the arduino/server has been running
//original code from Arduino forum
//https://forum.arduino.cc/t/function-uptime/70600
char *uptime(unsigned long milli) {
  static char _return[25];
  unsigned long secs = milli / 1000, mins = secs / 60;
  unsigned int hours = mins / 60, days = hours / 24;
  milli -= secs * 1000;
  secs -= mins * 60;
  mins -= hours * 60;
  hours -= days * 24;
  sprintf(_return, "%d days %2.2d:%2.2d:%2.2d.%3.3d", (byte)days, (byte)hours, (byte)mins, (byte)secs, (int)milli);
  return _return;
}

//convert int status to descriptive string for WiFi Status
char *WiFi_Status(int status_int) {
  static char _return[25];
  switch (status_int) {
    case 255:
      sprintf(_return, "WL_NO_SHIELD");
      break;
    case 0:
      sprintf(_return, "WL_IDLE_STATUS");
      break;
    case 1:
      sprintf(_return, "WL_NO_SSID_AVAIL");
      break;
    case 2:
      sprintf(_return, "WL_SCAN_COMPLETED");
      break;
    case 3:
      sprintf(_return, "WL_CONNECTED");
      break;
    case 4:
      sprintf(_return, "WL_CONNECT_FAILED");
      break;
    case 5:
      sprintf(_return, "WL_CONNECTION_LOST");
      break;
    case 6:
      sprintf(_return, "WL_DISCONNECTED");
      break;
    default:
      sprintf(_return, "ERROR");
      break;
  }
  return _return;
}

/*
  setup ethernet connection pada function setup
*/
void connectEthernet(){
  // mulai koneksi
  Serial.println("Initialize Ethernet with DHCP:");
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    } else if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("Ethernet cable is not connected.");
    }
    // no point in carrying on, so do nothing forevermore:
    while (true) {
      delay(500);
      Serial.print(".");
    }
  }
  // print your local IP address:
  Serial.print("My IP address: ");
  Serial.println(Ethernet.localIP());
}

/*
  function untuk menjaga agar koneksi ethernet tetap berjalan pada function loop
*/
void maintainEthernet(){
  switch (Ethernet.maintain()) {
    case 1:
      //renewed fail
      Serial.println("Error: renewed fail");
      break;

    case 2:
      //renewed success
      Serial.println("Renewed success");
      //print your local IP address:
      Serial.print("My IP address: ");
      Serial.println(Ethernet.localIP());
      break;

    case 3:
      //rebind fail
      Serial.println("Error: rebind fail");
      break;

    case 4:
      //rebind success
      Serial.println("Rebind success");
      //print your local IP address:
      Serial.print("My IP address: ");
      Serial.println(Ethernet.localIP());
      break;

    default:
      //nothing happened
      break;
  }
}



//////////////////////////////////////////////////////////////////////////////////////////
// Start Setup                                                                          //
//////////////////////////////////////////////////////////////////////////////////////////

void setup() {
  delay(1000);  //pause before starting serial, wihtout this, the esp32 will need the reset button pressed after flashing to get the serial monitor working
  MySerial.begin(9600);
  //while (!MySerial && (millis() < 5000)) delay(10); //does not work on esp32
  MySerial.println("Begin Setup...");


  //start the Ethernet connection and the server:
  Ethernet.init(5);  //set the CS pin, required for ESP32 as the arduino default is different
  Ethernet.begin(mac);
  // Ethernet.begin(mac, ip);

  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    MySerial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    berhasil = false;
    while (true) {
      delay(1);  // do nothing, no point running without Ethernet hardware
    }
    delay(1);
  } else if (Ethernet.linkStatus() == LinkOFF) {
    MySerial.println("Ethernet cable is not connected.");
    berhasil = false;
  } else {
    berhasil = true;
    // start the server
    server.begin();
    MySerial.print(F("Server is at "));
    MySerial.println(Ethernet.localIP());
    // connectEthernet();
    //don't need wifi since we are using an ethernet adapter, turn it off
    WiFi.disconnect();
    WiFi.mode(WIFI_OFF);
    MySerial.print("WiFi Status: ");
    //MySerial.println(WiFi.status());  //should output 255 indicating no shield (meaning it is disabled)
    MySerial.println(WiFi_Status(WiFi.status()));  //this will return a descriptive string instead of a int but because of the assoicated function, uses more memory than the line above
    // WL_NO_SHIELD        = 255,
    // WL_IDLE_STATUS      = 0,
    // WL_NO_SSID_AVAIL    = 1,
    // WL_SCAN_COMPLETED   = 2,
    // WL_CONNECTED        = 3,
    // WL_CONNECT_FAILED   = 4,
    // WL_CONNECTION_LOST  = 5,
    // WL_DISCONNECTED     = 6,



    MySerial.println("Setup Complete.");
    MySerial.println("");
  }

  // if (Ethernet.linkStatus() == LinkOFF) {
    // MySerial.println("Cn");
  // }

  //  berhasil = true;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Start Main Loop                                                                      //
//////////////////////////////////////////////////////////////////////////////////////////

void loop() {
  // maintainEthernet();
  if (berhasil == true) {
    Serial.println("BISA");
    delay(3000);
  } else {
    Serial.println("GAGAL");
    delay(3000);
  }
}