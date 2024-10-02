//rename serial, helpful when using multiple serial connections
#define MySerial Serial
#include <ArduinoJson.h>
#include <ArduinoJson.hpp>
#include <HTTPClient.h>
#include <SPI.h>
//library RFID
#include <MFRC522.h>
#include <Wire.h>
#include <TFT_eSPI.h>       // Hardware-specific library
#include <EthernetESP32.h>  //library modifikasi dari Ethernet.h
#include "AudioGeneratorAAC.h"
#include "AudioOutputI2S.h"
#include "AudioFileSourcePROGMEM.h"
#include "sudah_scan.h"
#include "gagal.h"
#include "0.h"
#include "1.h"
#include "2.h"
#include "3.h"
#include "4.h"
#include "5.h"
#include "6.h"
#include "7.h"
#include "8.h"
#include "9.h"
#include "kupon_used.h"
#include "salah_kantin.h"
#include "berhasil.h"
#include "beep.h"

#define Bit_Clock_BCLK 27
#define Word_Select_WS 26
#define Serial_Data_SD 25
#define GAIN 0.4

AudioFileSourcePROGMEM *in;
AudioGeneratorAAC *aac;
AudioOutputI2S *out;

// #include <Ethernetclient.h>
// #include "defines.h"
// You must have SSL Certificates here
// #include "trust_anchors.h"
//esp library ETH does not work with the w5500
byte mac[] = { 0x74, 0x69, 0x69, 0x2D, 0x32, 0x33 };  // ethernet mac address - harus unik <perbarui untuk setiap kode esp>

String readString;  //var for url requested, this allows us to serve different pages based on the url
// String API_HOST = "https://freetestapi.com/api/v1/users/1";  // Host API
// String API_HOST = "blbqfx2p-80.asse.devtunnels.ms";  // Host API
String API_HOST = "192.168.137.1";  // Host API
String API_MAPPING = "/ProyekKupon/api/mapping_arduino_katering/select";
String API_RFID = "/ProyekKupon/api/rfid/select";
String API_TRANSACTION = "/ProyekKupon/api/kupon_harian/insert";
String API_ARDUINO = "/ProyekKupon/api/arduino/insert";
String API_KUPON = "/ProyekKupon/api/kupon_harian/select";
// extern SPIClass SPI2;
//init Ethernet
EthernetServer server(80);  //set HTTP server port
EthernetClient client;
// Ethernetclient client(client, TAs, (size_t)TAs_NUM);
/*
variabel untuk menyimpan data id kantin & katering saat mencari
dengan mapping_arduino_katering
*/
int id_katering;
int id_kantin;
int error_code;
String katering;

//untuk menyimpan bacaan rfid
String uidCard;
//untuk menyimpan isp_arduino
String ip_arduino;
//variabel untuk id unik alat
String id_arduino = "GR 45 T";
// Pin GPIO yang terhubung ke LED
const int ledPin = 14;

int nik;
String status;
String error;
int jumlah = 0;

TFT_eSPI tft = TFT_eSPI();  // Invoke custom library

#define TFT_GREY 0x5AEB  // New colour


//konfigurasi scs w5500
#define W5500_CS 21

//konfigurasi pin untuk rfid
#define RST_PIN 22
#define MFRC522_CS 5
//inisiasi rfid
MFRC522 mfrc522(MFRC522_CS, RST_PIN);
bool berhasil_ = false;
/*
/////////////////////////////////////////
//           W5500 - ESP32            //
///////////////////////////////////////
VSPI 
5V - No Connection (NC)
GND - GND 
RST - NC
INT - NC
GND - NC
3.3V - 3.3V 
MISO - GPIO 19
MOSI - GPIO 23
SCS - GPIO 21
SCLK - GPIO 18



/////////////////////////////////////////
//           MFRC522 - ESP32            //
///////////////////////////////////////

//HSPI MFRC522
RST: GPIO 22
SDA (CS): GPIO 5
SCK (SCLK): GPIO 18
MOSI: GPIO 23
MISO: GPIO 19
IRQ: (optional) NC

SPI	MOSI	MISO	SCK	CS
HSPI	GPIO 13	GPIO 12	GPIO 14	GPIO 15
VSPI	GPIO 23	GPIO 19	GPIO 18	GPIO 5
18, 19, 23, cs
*/


// int count = 0;

#include <stdint.h>
// #include "image_data_gl.h"
#include "image_data_kk.h"



void setup() {

  MySerial.begin(9600);

  // out = new AudioOutputI2S();
  // out->SetGain(GAIN);
  // out->SetPinout(Bit_Clock_BCLK, Word_Select_WS, Serial_Data_SD);
  // aac = new AudioGeneratorAAC();

  SPI.begin(18, 19, 23, MFRC522_CS);
  tft.init();
  tft.setRotation(1);
  tft.setSwapBytes(true);


  // // Mengatur pin sebagai output
  pinMode(ledPin, OUTPUT);
  pinMode(MFRC522_CS, OUTPUT);
  pinMode(W5500_CS, OUTPUT);

  Ethernet.init(W5500_CS);  // Inisialisasi Ethernet
  bool isConnected = connect_ethernet();

  if (isConnected) {
    MySerial.println("===TERHUBUNG KE ETHERNET===");
    MySerial.println("Dekatkan kartu ke reader!");
    getMapping(id_arduino);
    delay(400);
    getTransaksiPerDay(id_arduino);
    delay(400);
    showScreen(katering, jumlah, nik, status, error);
  } else {
    MySerial.println("===TIDAK TERHUBUNG ETHERNET===");
  }

  // // Inisialisasi MFRC522
  mfrc522.PCD_Init();  // Inisialisasi MFRC522
  out = new AudioOutputI2S();
  out->SetGain(GAIN);
  out->SetPinout(Bit_Clock_BCLK, Word_Select_WS, Serial_Data_SD);

  aac = new AudioGeneratorAAC();
}

void loop() {

  Ethernet.maintain();

  String rfid_uid = readRfid();

  if (rfid_uid != "") {
    Serial.println(rfid_uid);
  }

  showScreen(katering, jumlah, nik, status, error);
  delay(700);
}




/*
  function untuk connect ke ethernet dan mendapatkan IP address dari router/laptop/switch
*/
bool connect_ethernet() {
  MySerial.println("Begin Setup...");

  MySerial.println("====GETTING IP ADDRESS===");
  Ethernet.begin(mac);
  // Check for Ethernet hardware present
  if (Ethernet.hardwareStatus() == EthernetNoHardware) {
    MySerial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    berhasil_ = false;
    while (true) {
      delay(1);  // do nothing, no point running without Ethernet hardware
    }
    delay(1);
  } else if (Ethernet.linkStatus() == LinkOFF) {
    MySerial.println("Ethernet cable is not connected.");
    berhasil_ = false;
  } else {
    berhasil_ = true;
    // start the server
    server.begin();
    //tampilkan ip address ESP32
    MySerial.print(F("Server is at "));
    MySerial.println(Ethernet.localIP());

    MySerial.println("Setup Complete.");
    MySerial.println("");
  }
  return berhasil_;
}



/*
  function untuk membaca UID card
  Args:
    void
  returns:
    String content : mengembalikan string UID kartu  
*/
String readRfid() {

  //deteksi apakah ada kartu atau select jika ada kartu
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    return "";
  }

  playSound(beep, sizeof(beep));

  //membaca uid
  Serial.print("UID tag : ");
  String content = "";

  for (uint8_t i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }

  Serial.println();
  Serial.print("UID value : ");
  Serial.println(content);
  //handle jika user sudah melakukan scan
  if (uidCard == content) {
    Serial.println("Sudah Scan");
    //play sudah scan 
    playSound(sudah_scan, sizeof(sudah_scan));

    digitalWrite(ledPin, HIGH);
    delay(100);
    digitalWrite(ledPin, LOW);
    delay(100);
    digitalWrite(ledPin, HIGH);
    delay(100);
    digitalWrite(ledPin, LOW);
    delay(100);
    showScreen(katering, jumlah, nik, "gagal", "Sudah Scan");
  } else {
    uidCard = content;
    // insertArduino(content);
    // digitalWrite(W5500_CS, LOW);  // Aktifkan W5500
    // Proses Ethernet
    // dummyContoh();
    bool get_map = getMapping(id_arduino);

    if (get_map == true) {
      sendTransaksi(id_arduino, id_kantin, id_katering, content);
      Serial.println("===TERKIRIM===");
      Serial.println("id_Arduino : " + String(id_arduino) + "id_kantin : " + String(id_kantin) + "id_katering : " + String(id_katering) + "RFID : " + String(content));
      getTransaksiPerDay(id_arduino);
      if(status == "berhasil"){
          playSound(berhasil, sizeof(berhasil));
          playAudioFromString(String(jumlah));
      }else{
        if(error == "1"){
          playSound(kupon_used, sizeof(kupon_used));
        }else{
          playSound(salah_kantin, sizeof(salah_kantin));
        }
      }
      // if (aac->isRunning()) {
      //   aac->loop();
      // } else {
      //   delete in;
      //   in = new AudioFileSourcePROGMEM(kantin_natpro, sizeof(kantin_natpro));
      //   aac->begin(in, out);
      //   while (aac->isRunning()) {
      //     aac->loop();
      //   }
      //   delay(2000);
      // }
    }
    // sendTransaksi("AD 12T 5", 1, 1, content);
    // digitalWrite(W5500_CS, HIGH);
  }
  return content;
}




/*
 function untuk get data dari mapping_arduino_katering, 
 untuk mencari id_kantin dan id_katering menggunakan id_arduino.
 Args:
  id_arduino : mencari data id_kantin & id_katering pada mapping_arduino_katering (select)

 returns:
  boolean : kondisi jika data berhasil dikirim 

*/
bool getMapping(String id_arduino) {
  bool condition = false;
  if (Ethernet.linkStatus() == 1) {
    if (client.connect(API_HOST.c_str(), 80)) {
      MySerial.println("Connected to server");

      // Data yang akan dikirimkan dalam format JSON
      String postData = "{\"id_arduino\":\"" + id_arduino + "\"}";

      // Buat request HTTP POST
      client.println("POST " + API_MAPPING + " HTTP/1.1");
      client.println("Host: " + API_HOST);
      client.println("Content-Type: application/json");
      client.println("Connection: close");
      client.print("Content-Length: ");
      client.println(postData.length());
      client.println();
      client.println(postData);

      // Baca response dari server
      while (client.connected()) {
        String line = client.readStringUntil('\n');
        if (line == "\r") {
          MySerial.println("Headers received");
          break;
        }
      }

      // Baca body response
      String responseBody = client.readString();
      MySerial.println("Response body: " + responseBody);

      //consume json response dari api
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, responseBody);

      //apabila data diterima tidak null, maka simpan ke variabel terkait
      if (doc["id_katering"] != "") {
        id_katering = doc["id_katering"].as<int>();
        id_kantin = doc["id_kantin"].as<int>();
        katering = doc["katering"].as<String>();
      }

      if (id_katering != 0 && id_kantin != 0) {
        condition = true;
      }

      client.stop();  // Tutup koneksi
    } else {
      MySerial.println("Connection to server failed");
      condition = false;
    }
  }

  // delay(1000);
  return condition;
}


bool getTransaksiPerDay(String id_arduino) {
  bool condition = false;
  if (Ethernet.linkStatus() == 1) {
    if (client.connect(API_HOST.c_str(), 80)) {
      MySerial.println("Connected to server");

      // Data yang akan dikirimkan dalam format JSON
      String postData = "{\"id_arduino\":\"" + id_arduino + "\"}";

      // Buat request HTTP POST
      client.println("POST " + API_KUPON + " HTTP/1.1");
      client.println("Host: " + API_HOST);
      client.println("Content-Type: application/json");
      client.println("Connection: close");
      client.print("Content-Length: ");
      client.println(postData.length());
      client.println();
      client.println(postData);

      // Baca response dari server
      while (client.connected()) {
        String line = client.readStringUntil('\n');
        if (line == "\r") {
          MySerial.println("Headers received");
          break;
        }
      }

      // Baca body response
      String responseBody = client.readString();
      MySerial.println("Response body: " + responseBody);

      //consume json response dari api
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, responseBody);

      //apabila data diterima tidak null, maka simpan ke variabel terkait
      if (doc["status"] != "") {
        jumlah = doc["jumlah"].as<int>();
        nik = doc["nik"].as<int>();
        status = doc["status"].as<String>();
        error = doc["error"].as<String>();
        condition = true;
      }

        client.stop();  // Tutup koneksi
      }
      else {
        MySerial.println("Connection to server failed");
        condition = false;
      }
    }

    // delay(1000);
    return condition;
  }


  void showScreen(String nama_katering, int jumlah, int nik, String status, String error) {

    tft.fillScreen(TFT_BLACK);
    // tft.pushImage(0, 0, 480, 320, image_data_gl);
    tft.pushImage(tft.width() - 100, 10, 100, 21, image_data_ko);

    if (jumlah > 0) {
      int16_t textWidth = tft.textWidth(nama_katering, 3);  // font 2
      // int16_t textHeight = tft.textHeight(nama_katering, 3);

      int16_t centerX = (tft.width() - textWidth) / 2;
      // int16_t centerY = (tft.height() - textHeight) / 2;
      tft.setCursor(20, 10);
      tft.setTextSize(1);
      tft.setTextColor(TFT_BLACK, TFT_YELLOW);
      tft.setTextFont(4);
      tft.print("Katering ");
      tft.println(nama_katering);

      tft.setCursor(tft.width() - 130, 50);
      tft.setTextSize(1);
      tft.setTextColor(TFT_WHITE);
      tft.setTextFont(4);
      tft.print("NIK : ");
      tft.println(nik);
      // tft.println(nama_katering);

      int16_t textWidth_count = tft.textWidth(String(jumlah), 7);
      int16_t centerX_cout = (tft.width() - (textWidth_count + 40)) / 2;
      tft.setCursor(60, 70, 7);
      tft.setTextSize(3);
      tft.setTextFont(7);
      tft.setTextColor(TFT_WHITE);
      // tft.println("300");
      tft.println(jumlah);

      tft.setCursor(10, tft.height() - 100, 7);
      tft.setTextSize(1);
      tft.setTextFont(4);


      if (status == "berhasil") {

        tft.setCursor(10, tft.height() - 80, 7);
        tft.setTextSize(1);
        tft.setTextColor(TFT_WHITE, TFT_GREEN);
        tft.setTextFont(4);
        tft.println(F("BERHASIL"));
        // delay(2000);
      } else {

        if (error == "1") {
          error = "Kupon sudah digunakan";
        } else {
          error = "Salah Kantin";
        }

        tft.setCursor(10, tft.height() - 80, 7);
        tft.setTextSize(1);
        tft.setTextColor(TFT_WHITE, TFT_RED);
        tft.setTextFont(4);
        tft.print(F("GAGAL "));
        tft.println(error);
      }
    } else {
      int16_t textWidth_count = tft.textWidth("READY!", 7);
      int16_t centerX_cout = (tft.width() - textWidth_count) / 2;
      tft.setCursor(centerX_cout, 55, 7);

      tft.setCursor(centerX_cout, 50);
      tft.setTextSize(2);
      tft.setTextColor(TFT_WHITE);
      tft.setTextFont(4);
      tft.println("READY!");
    }


    delay(1000);
  }



  /*
  function untuk post data transaksi kupon_harian
  Args:
    String id_ardino
    INT id_kantin
    INT id_katering
    String no_rfid
  returns :
    boolean: jika data berhasil di post  
*/
  bool sendTransaksi(String id_arduino, int id_kantin, int id_katering, String no_rfid) {
    if (Ethernet.linkStatus() == LinkON) {
      if (client.connect(API_HOST.c_str(), 80)) {
        MySerial.println("Connected to server");
        // Trim left pada no_rfid (menghapus spasi di depan)
        while (no_rfid.length() > 0 && no_rfid[0] == ' ') {
          no_rfid.remove(0, 1);  // hapus karakter pertama jika spasi
        }

        // Data yang akan dikirimkan dalam format JSON
        String postData = "{\"id_arduino\":\"" + id_arduino + "\","
                                                              "\"id_kantin\":"
                          + String(id_kantin) + ","
                                                "\"id_katering\":"
                          + String(id_katering) + ","
                                                  "\"no_rfid\":\""
                          + no_rfid + "\"}";


        // Buat request HTTP POST
        client.println("POST " + API_TRANSACTION + " HTTP/1.1");
        client.println("Host: " + API_HOST);
        client.println("Content-Type: application/json");
        client.println("Connection: close");
        client.print("Content-Length: ");
        client.println(postData.length());
        client.println();
        client.println(postData);

        // Baca response dari server
        while (client.connected()) {
          String line = client.readStringUntil('\n');
          if (line == "\r") {
            MySerial.println("Headers received");
            break;
          }
        }
        digitalWrite(ledPin, LOW);
        delay(300);
        digitalWrite(ledPin, HIGH);
        // Baca body response
        String responseBody = client.readString();
        MySerial.println("Response body: " + responseBody);

        return true;
      } else {
        MySerial.println("Connection to server transaksi failed");
        return false;
      }
      int size;
      while ((size = client.available()) > 0) {
        Serial.print("Size is: ");
        Serial.println(size);
        uint8_t *msg = (uint8_t *)malloc(size + 1);
        memset(msg, 0, size + 1);
        size = client.read(msg, size);
        Serial.print("Size1 is: ");
        Serial.println(size);
        Serial.write(msg, size);
        free(msg);
      }
      client.stop();  // Tutup koneksi
      // Ethernet.maintain();
      return false;
    }
    // delay(1000);
    return false;
  }


void playSound(const void *data, uint32_t len){
 if (aac->isRunning()) {
      aac->loop();
    } else {
      delete in;
      in = new AudioFileSourcePROGMEM(data, len);
      aac->begin(in, out);
      while (aac->isRunning()) {
        aac->loop();
      }
      delay(800);
    }   
}


void playAudioFromString(String s) {
  for (int i = 0; i < s.length(); i++) {
    char character = s[i]; // Ambil setiap karakter
    Serial.print("Playing audio for character: ");
    Serial.println(character);

    // Pilih file audio berdasarkan karakter
    switch (character) {
      case '0':
        in = new AudioFileSourcePROGMEM(_0, sizeof(_0));  // Ganti dengan path file audio di SD card
        aac->begin(in, out);
        break;
      case '1':
        in = new AudioFileSourcePROGMEM(_1, sizeof(_1));  // Ganti dengan path file audio di SD card
        aac->begin(in, out);
        break;
      
      case '2':
        in = new AudioFileSourcePROGMEM(_2, sizeof(_2));  // Ganti dengan path file audio di SD card
        aac->begin(in, out);
        break;
      
      case '3':
        in = new AudioFileSourcePROGMEM(_3, sizeof(_3));  // Ganti dengan path file audio di SD card
        aac->begin(in, out);
        break;
      case '4':
        in = new AudioFileSourcePROGMEM(_4, sizeof(_4));  // Ganti dengan path file audio di SD card
        aac->begin(in, out);
        break;
      case '5':
        in = new AudioFileSourcePROGMEM(_5, sizeof(_5));  // Ganti dengan path file audio di SD card
        aac->begin(in, out);
        break;
      case '6':
        in = new AudioFileSourcePROGMEM(_6, sizeof(_6));  // Ganti dengan path file audio di SD card
        aac->begin(in, out);
        break;
      case '7':
        in = new AudioFileSourcePROGMEM(_7, sizeof(_7));  // Ganti dengan path file audio di SD card
        aac->begin(in, out);
        break;
      case '8':
        in = new AudioFileSourcePROGMEM(_8, sizeof(_8));  // Ganti dengan path file audio di SD card
        aac->begin(in, out);
        break;
      case '9':
        in = new AudioFileSourcePROGMEM(_9, sizeof(_9));  // Ganti dengan path file audio di SD card
        aac->begin(in, out);
        break;
      
    }

    while(aac->isRunning()){
        aac->loop();
    }

    // aac->stop();
    // delete in;   
  }
}
