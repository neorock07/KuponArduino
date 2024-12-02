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
//audio library dan audio file
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
#include "connected.h"
#include "kartu_tidak_terdaftar.h"
/*
  import array image logo konimex
*/
#include <stdint.h>
#include "image_data_kk.h"


/*
  kode untuk set pin pada modul amplifier MAX98357A
*/
#define Bit_Clock_BCLK 27
#define Word_Select_WS 26
#define Serial_Data_SD 25
#define GAIN 1

AudioFileSourcePROGMEM *in;
AudioGeneratorAAC *aac;
AudioOutputI2S *out;

// ethernet mac address - harus unik <perbarui untuk setiap kode esp>
byte mac[] = { 0x74, 0x69, 0x69, 0x2D, 0x32, 0x33 };

String readString;  //var for url requested, this allows us to serve different pages based on the url

String API_HOST = "192.168.0.119";  // Host API
String API_MAPPING = "/proyekkuponmakan/api/mapping_arduino_katering/select";
String API_RFID = "/proyekkuponmakan/api/rfid/select";
String API_TRANSACTION = "/proyekkuponmakan/api/kupon_harian/insert";
String API_ARDUINO = "/proyekkuponmakan/api/arduino/insert";
String API_KUPON = "/proyekkuponmakan/api/kupon_harian/select";
//init Ethernet
EthernetServer server(80);  //set HTTP server port
EthernetClient client;

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
const int ledPin = 13;

/*
  untuk menyimpan data setelah getTransaksiPerDay() dijalankan
*/
String nik;
String status;
String error;
int jumlah = 0;

//inisiasi objek untuk LCD TFT
TFT_eSPI tft = TFT_eSPI();

//konfigurasi pin CS modul W5500 ethernet
#define W5500_CS 21

//konfigurasi pin untuk rfid modul
#define RST_PIN 22
#define MFRC522_CS 5

//inisiasi rfid
MFRC522 mfrc522(MFRC522_CS, RST_PIN);
bool berhasil_ = false;

/*
////////////////////////////////////////////////////
//                KONFIGURASI PIN                //
//////////////////////////////////////////////////

/////////////////////////////////////////
//           W5500 - ESP32            //
///////////////////////////////////////

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

RST: GPIO 22
SDA (CS): GPIO 5
SCK (SCLK): GPIO 18
MOSI: GPIO 23
MISO: GPIO 19
IRQ: (optional) NC


////////////////////////////////////////////
//              LCD TFT                  //
//////////////////////////////////////////

MISO - 19
MOSI - 23
SCLK - 18
CS -  15   // Chip select control pin
DC  - 14   // Data Command control pin
RST - 17   // Reset pin (could connect to RST pin)
TOUCH_CS - 4     // Chip select pin (T_CS) of touch screen
LED - 3.3V

/////////////////////////////////////////
//              MAX98357A SPEAKER     //
///////////////////////////////////////
LRC - 26
BCLK - 27
DIN - 25
GAIN - GND
SD - NOT CONNECTED



SPI	MOSI	MISO	SCK	CS
-------------------------------------
HSPI	GPIO 13	GPIO 12	GPIO 14	GPIO 15
VSPI	GPIO 23	GPIO 19	GPIO 18	GPIO 5
-------------------------------------

NOTE ** :
 pakai VSPI Bus semua : -> 18, 19, 23, CS

*/




void setup() {

  MySerial.begin(9600);
  Serial.println("heap :");
  Serial.println(esp_get_free_heap_size());
  //inisiasi SPI Bus pada VSPI
  SPI.begin(18, 19, 23, MFRC522_CS);
  tft.init();
  tft.setRotation(3);
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
  playSound(connected, sizeof(connected));
}

void loop() {
  Ethernet.maintain();
  String rfid_uid = readRfid();
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
  // Serial.print("UID tag : ");
  String content = "";

  for (uint8_t i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }

  //handle jika user sudah melakukan scan
  if (uidCard == content) {
    //play sudah scan
    playSound(sudah_scan, sizeof(sudah_scan));
    showScreen(katering, jumlah, nik, "gagal", "Sudah Scan");
  } else {
    uidCard = content;
    /*
        kirim transaksi ke server
      */
    sendTransaksi(id_arduino, id_kantin, id_katering, content);
    /*
        cek hasil respon dari server terhadap data transaksi yang diterima, 
        jika berhasil maka putar audio berhasil, jika gagal maka putar
        audio sesuai kode error nya.
      */
    if (status == "berhasil") {
      playSound(berhasil, sizeof(berhasil));
      playAudioFromString(String(jumlah));
      showScreen(katering, jumlah, nik, status, error);

    } else {
      if (error == "1") {
        playSound(kupon_used, sizeof(kupon_used));
      } else if(error == "2"){
        playSound(salah_kantin, sizeof(salah_kantin));
      } 
      else {
        playSound(kartu_tidak_terdaftar, sizeof(kartu_tidak_terdaftar));
      }
      showScreen(katering, jumlah, nik, status, error);
    }
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
      // MySerial.println("Connected to server");

      // Data yang akan dikirimkan dalam format JSON
      String postData = "{\"id_arduino\":\"" + id_arduino + "\"}";

      // Buat request HTTP POST
      client.println("POST " + API_MAPPING + " HTTP/1.0");
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
          // MySerial.println("Headers received");
          break;
        }
      }

      // Baca body response
      String responseBody = client.readString();

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
  return condition;
}

/*
  Function untuk mendapatkan data transaksi per hari.
  params:
    id_arduino (str) : inputan id arduino masing-masing
  returns:
    condition (boolean) : true/false kondisi request ke server;
*/
bool getTransaksiPerDay(String id_arduino) {
  bool condition = false;
  if (Ethernet.linkStatus() == 1) {
    if (client.connect(API_HOST.c_str(), 80)) {

      // Data yang akan dikirimkan dalam format JSON
      String postData = "{\"id_arduino\":\"" + id_arduino + "\"}";

      // Buat request HTTP POST
      client.println("POST " + API_KUPON + " HTTP/1.0");
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
          break;
        }
      }

      // Baca body response
      String responseBody = client.readString();

      //consume json response dari api
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, responseBody);

      //apabila data diterima tidak null, maka simpan ke variabel terkait
      if (doc["status"] != "") {
        jumlah = doc["jumlah"].as<int>();
        nik = doc["nik"].as<String>();
        status = doc["status"].as<String>();
        error = doc["error"].as<String>();
        condition = true;
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
  return condition;
}

/*
    function untuk membuat tata letak LCD
    Args:
      nama_katering (str) : nama_katering sesuai id_arduino
      jumlah (int) : jumlah transaksi penjualan
      nik (string) : nik pengguna
      status (str) : status transaksi (berhasil, gagal);
      error (str) : jenis error (0, 1, 2, 3 ) | 0=berhasil;1=sudah digunakan;2=salah kantin;3=kartu tidak terdaftar;
  */
void showScreen(String nama_katering, int jumlah, String nik, String status, String error) {

  tft.fillScreen(TFT_BLACK);
  // tft.pushImage(0, 0, 480, 320, image_data_gl);
  tft.pushImage(tft.width() - 100, 10, 100, 21, image_data_ko);

  if (jumlah > 0) {
    int16_t textWidth = tft.textWidth(nama_katering, 3);  // font 2

    int16_t centerX = (tft.width() - textWidth) / 2;
    tft.setCursor(20, 10);
    tft.setTextSize(1);
    tft.setTextColor(TFT_BLACK, TFT_YELLOW);
    tft.setTextFont(4);
    tft.print("Katering ");
    tft.println(nama_katering);

    tft.setCursor(tft.width() - 140, 50);
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
    } else {

      if (error == "1") {
        error = "Kupon sudah digunakan";
      } else if (error == "2") {
        error = "Salah Kantin";
      } else {
        error = "Kartu Belum Terdaftar";
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

  delay(300);
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
      // MySerial.println("Connected to server");
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
      client.println("POST " + API_TRANSACTION + " HTTP/1.0");
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
          // MySerial.println("Headers received");
          break;
        }
      }

      // Baca body response
      String responseBody = client.readString();

      //consume json response dari api
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, responseBody);

      //apabila data diterima tidak null, maka simpan ke variabel terkait
      if (doc["status"] != "") {
        jumlah = doc["jumlah"].as<int>();
        nik = doc["nik"].as<String>();
        status = doc["status"].as<String>();
        error = doc["error"].as<String>();
        return true;
      }
    } else {
      MySerial.println("Connection to server transaksi failed");
      return false;
    }
    client.stop();  // Tutup koneksi
    return false;
  }
  return false;
}

/*
  Function untuk play sound
  Args:
    *data (pointer array) : array dari hasil konversi file audio .aac (.aac -> char array.H)
    len (int) : panjang array  
*/
void playSound(const void *data, uint32_t len) {
  if (aac->isRunning()) {
    aac->loop();
  } else {
    delete in;
    in = new AudioFileSourcePROGMEM(data, len);
    aac->begin(in, out);
    while (aac->isRunning()) {
      aac->loop();
    }
    // delay(1);
  }
}


/*
  function untuk play sound khusus untuk jumlah transaksi.
  Args:
    s (str) : jumlah transaksi berhasil
*/
void playAudioFromString(String s) {
  for (int i = 0; i < s.length(); i++) {
    char character = s[i];  // Ambil setiap karakter
    Serial.print("Playing audio for character: ");
    Serial.println(character);

    // Pilih file audio berdasarkan karakter
    switch (character) {
      case '0':
        in = new AudioFileSourcePROGMEM(_0, sizeof(_0));  
        aac->begin(in, out);
        break;
      case '1':
        in = new AudioFileSourcePROGMEM(_1, sizeof(_1));  
        aac->begin(in, out);
        break;

      case '2':
        in = new AudioFileSourcePROGMEM(_2, sizeof(_2));  
        aac->begin(in, out);
        break;

      case '3':
        in = new AudioFileSourcePROGMEM(_3, sizeof(_3));  
        aac->begin(in, out);
        break;
      case '4':
        in = new AudioFileSourcePROGMEM(_4, sizeof(_4));  
        aac->begin(in, out);
        break;
      case '5':
        in = new AudioFileSourcePROGMEM(_5, sizeof(_5));  
        aac->begin(in, out);
        break;
      case '6':
        in = new AudioFileSourcePROGMEM(_6, sizeof(_6));  
        aac->begin(in, out);
        break;
      case '7':
        in = new AudioFileSourcePROGMEM(_7, sizeof(_7));  
        aac->begin(in, out);
        break;
      case '8':
        in = new AudioFileSourcePROGMEM(_8, sizeof(_8));  
        aac->begin(in, out);
        break;
      case '9':
        in = new AudioFileSourcePROGMEM(_9, sizeof(_9));  
        aac->begin(in, out);
        break;
    }

    while (aac->isRunning()) {
      aac->loop();
    }
  }
}
