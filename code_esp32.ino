#include <ArduinoJson.h>
#include <ArduinoJson.hpp>

#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <SPI.h>
#include <MFRC522.h>
#include "Audio.h"
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
/*
SDA (SS) ke GPIO 5 (D5)
SCK ke GPIO 18 (D18)
MOSI ke GPIO 23 (D23)
MISO ke GPIO 19 (D19)
IRQ (tidak terhubung)
GND ke GND
RST ke GPIO 22 (D22)
3.3V ke 3.3V
*/


//konfigurasi pin untuk rfid
#define RST_PIN 22
#define SS_PIN 5

//alamat API untuk select mapping_arduino_katering
#define API_MAPPING "https://blbqfx2p-80.asse.devtunnels.ms/ProyekKupon/api/mapping_arduino_katering/select"
//alamat API untuk insert kupon_harian
#define API_TRANSACTION "https://blbqfx2p-80.asse.devtunnels.ms/ProyekKupon/api/kupon_harian/insert"
//alamat API untuk insert arduino
#define API_ARDUINO "https://blbqfx2p-80.asse.devtunnels.ms/ProyekKupon/api/arduino/insert"
#define API_RFID "https://blbqfx2p-80.asse.devtunnels.ms/ProyekKupon/api/rfid/select"


const char* ssid = "Infinix HOT 8 Neo ";
const char* password = "07072003";

//variabel untuk id unik alat
String id_arduino = "GR 45 T";
WiFiClient wifiClient;
/*
variabel untuk menyimpan data id kantin & katering saat mencari
dengan mapping_arduino_katering
*/
int id_katering;
int id_kantin;

//untuk menyimpan bacaan rfid
String uidCard;
//untuk menyimpan ip_arduino
String ip_arduino;


const int ledPin = 2;  // Pin GPIO yang terhubung ke LED
//inisiasi rfid
MFRC522 mfrc522(SS_PIN, RST_PIN);

WiFiClientSecure client;

void setup() {
  Serial.begin(115200);
  // Mengatur pin sebagai output
  pinMode(ledPin, OUTPUT);
  //inisiasi SPI bus
  SPI.begin();
  mfrc522.PCD_Init();
  //connect wifi
  connectWifi();

  //inisiasi mfrc522
  Serial.println("Dekatkan kartu ke reader!");
  Serial.print("");
}

void loop() {
  Serial.println("belum ada kartu");
  String rfid_uid = readRfid();
  if (rfid_uid != "") {
    // if(getMapping(id_arduino)){
    //   sendTransaksi(id_arduino, id_kantin, id_katering, 2);
    // }
    Serial.println(rfid_uid);
  }
  delay(2000);
}


/* 
fungsi untuk connect ke wifi
Args:
  void
returns:
  void    

*/
void connectWifi() {
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(ledPin, HIGH);
    delay(500);
    Serial.print(".");
    digitalWrite(ledPin, LOW);
  }
  digitalWrite(ledPin, LOW);
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  client.setInsecure();
  ip_arduino = WiFi.localIP().toString();
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

  //membaca uid
  Serial.print("UID tag : ");
  String content = "";

  for (uint8_t  i = 0; i < mfrc522.uid.size; i++) {
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
    digitalWrite(ledPin, HIGH);
    delay(100);
    digitalWrite(ledPin, LOW);
    delay(100);
    digitalWrite(ledPin, HIGH);
    delay(100);
    digitalWrite(ledPin, LOW);
    delay(100);
  } else {
    uidCard = content;
    // insertArduino(content);
    bool get_map = getMapping(id_arduino);

    if (get_map == true) {
      sendTransaksi(id_arduino, id_kantin, id_katering, content);
    }
    // sendTransaksi("AD 12T 5", 1, 1, content);
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
  //cek apakah tersambung wifi
  if (WiFi.status() == WL_CONNECTED) {
    StaticJsonDocument<200> jsonDoc;
    jsonDoc["id_arduino"] = id_arduino;

    HTTPClient http;
    //post data id_arduino ke API (select mapping_arduino_katering)
    http.begin(client, API_MAPPING);
    http.addHeader("Content-Type", "application/json");
    String jsonString;
    serializeJson(jsonDoc, jsonString);
    int responseCode = http.POST(jsonString);
    while(responseCode < 0){
      responseCode = http.POST(jsonString);
    }
    //cek jika berhasil post data
    if (responseCode == 200) {
      String response = http.getString();
      Serial.println("Data Mapping : " + response);

      //consume json response dari api
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, response);

      //apabila data diterima tidak null, maka simpan ke variabel terkait
      if (doc["id_katering"] != "") {
        id_katering = doc["id_katering"].as<int>();
        id_kantin = doc["id_kantin"].as<int>();
      }
      http.end();
      return true;
    }
    //jika post failed maka print error
    else {
      Serial.println("Error get mapping_arduino_katering : " + String(responseCode));
      http.end();
      return false;
    }
  }
  return false;
}


/*
function untuk select data id_rfid by no_rfid : (note: for testing purpose only)

Args : 
  no_rfid : String untuk mendapatkan data rfid berdasarkan no_rfid kartu

returns:
  boolean : kondisi jika post data terkirim
*/
bool getRfid(String no_rfid) {
  //cek apakah tersambung wifi
  if (WiFi.status() == WL_CONNECTED) {
    StaticJsonDocument<200> jsonDoc;
    no_rfid.trim();
    // Masukkan string yang telah di-trim ke dalam JSON document
    jsonDoc["no_rfid"] = no_rfid.c_str();
    Serial.println(no_rfid);

    HTTPClient http;
    //post data id_arduino ke API (select mapping_arduino_katering)
    http.begin(wifiClient, API_RFID);
    http.addHeader("Content-Type", "application/json");
    String jsonString;
    serializeJson(jsonDoc, jsonString);
    int responseCode = http.POST(jsonString);

    //cek jika berhasil post data
    if (responseCode == 200) {
      String response = http.getString();
      Serial.println("Data rfid : " + response);

      //consume json response dari api
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, response);

      //apabila data diterima tidak null, maka simpan ke variabel terkait
      if (doc["id_rfid"] != "") {
        Serial.println("Ada rfid nih");
      } else {
        Serial.println("tidak ada rfid");
      }
      http.end();
      return true;
    }
    //jika post failed maka print error
    else {
      Serial.println("Error get rfid : " + String(responseCode));
      http.end();
      return false;
    }
  }
  return false;
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
  //cek jika tersambung ke wifi
  if (WiFi.status() == WL_CONNECTED) {

    StaticJsonDocument<1024> jsonDoc;
    jsonDoc["id_arduino"] = String(id_arduino);
    jsonDoc["id_kantin"] = id_kantin;
    jsonDoc["id_katering"] = id_katering;
    no_rfid.trim();
    // Masukkan string yang telah di-trim ke dalam JSON document
    jsonDoc["no_rfid"] = no_rfid.c_str();
    Serial.println("Kudune ngirim sih...");


    HTTPClient http;
    //post data body json ke insert kupon_harian
    http.begin(client, API_TRANSACTION);
    http.addHeader("Content-Type", "application/json");
    String jsonString;
    serializeJson(jsonDoc, jsonString);
    int responseCode = http.POST(jsonString);
    while(responseCode < 0){
      responseCode = http.POST(jsonString);
    }
    //cek jika berhasil post data
    if (responseCode == 200) {

      String response = http.getString();
      Serial.println("Transaksi Response : " + response);

      //decode json hasil respon dan ke variabel local terkait;
      DynamicJsonDocument doc(1024);
      deserializeJson(doc, response);
      String message = doc["message"].as<String>();
      String kantin = doc["kantin"].as<String>();
      int nik = doc["nik"].as<int>();
      String status = doc["status"].as<String>();
      String error = doc["error"].as<String>();
      int jumlah = doc["jumlah"].as<int>();
      String tanggal = doc["tanggal"].as<String>();

      //print hasil response;
      Serial.println("Message : " + message);
      Serial.println("nik : " + String(nik));
      Serial.println("kantin : " + kantin);
      Serial.println("status : " + status);
      Serial.println("error : " + error);
      Serial.println("jumlah : " + String(jumlah));
      Serial.println("tanggal : " + tanggal);

      //return true jika berhasil post
      http.end();
      digitalWrite(ledPin, HIGH);
      delay(1000);
      digitalWrite(ledPin, LOW);  
      return true;
    }
    //cek jika tidak berhasil post data
    else {
      //print error & return false
      Serial.println("Error post transaksi : " + String(responseCode));
      http.end();
      return false;
    }
  }
  return false;
}



