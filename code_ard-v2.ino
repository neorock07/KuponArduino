#include <ArduinoJson.h>
#include <ArduinoJson.hpp>
#include <SPI.h>
#include <MFRC522.h>
#include <EEPROM.h>
#include <ESP8266HTTPClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>

/*
pin esp8266 :
SDA D2
SCK D5
MOSI D7
MISO D6
GND
RST D1
3V
*/


//konfigurasi pin untuk rfid
#define RST_PIN D1
#define SS_PIN D2

//alamat API untuk select mapping_arduino_katering
#define API_MAPPING "http://192.168.43.251/ProyekKupon/api/mapping_arduino_katering/select"
//alamat API untuk insert kupon_harian
#define API_TRANSACTION "http://192.168.43.251/ProyekKupon/api/kupon_harian/insert"
//alamat API untuk insert arduino
#define API_ARDUINO "http://192.168.43.251/ProyekKupon/api/arduino/insert"
#define API_RFID "http://192.168.43.251/ProyekKupon/api/rfid/select"

//inisiasi rfid
MFRC522 mfrc522(SS_PIN, RST_PIN);

WiFiClient wifiClient;
//variabel untuk konek ke wifi
const char* ssid = "Infinix HOT 8 Neo ";
const char* password = "07072003";

//variabel untuk id unik alat
String id_arduino = "AD 12 5";
/*
variabel untuk menyimpan data id kantin & katering saat mencari
dengan mapping_arduino_katering
*/
int id_katering;
int id_kantin;

//untuk menyimpan bacaan rfid
String uidCard;

//led uji
int led = D8;
//untuk menyimpan ip_arduino
String ip_arduino;

void setup() {

  //inisiasi serial komunikasi ke 9600
  Serial.begin(9600);
  pinMode(led, OUTPUT);
  digitalWrite(led, LOW);
  //inisiasi SPI bus
  SPI.begin();
  connectWifi();

  //inisiasi mfrc522
  Serial.println("Dekatkan kartu ke reader!");
  mfrc522.PCD_Init();
  Serial.print("");
}

void loop() {
  /*
    note: id_rfid int, tapi rfid_uid String perlu diubah kedepan
  */
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

/* fungsi untuk connect ke wifi */
void connectWifi() {
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(led, HIGH);
    delay(500);
    Serial.print(".");
    digitalWrite(led, LOW);
  }
  digitalWrite(led, LOW);
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  ip_arduino = WiFi.localIP().toString();
}


/*
  fungsi untuk membaca katu ketika user tap-card
*/
String readRfid() {

  //deteksi apakah ada kartu atau select jika ada kartu
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    return "";
  }

  //membaca uid
  Serial.print("UID tag : ");
  String content = "";
  byte letter;

  for (byte i = 0; i < mfrc522.uid.size; i++) {
    Serial.print(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " ");
    Serial.print(mfrc522.uid.uidByte[i], HEX);
    content.concat(String(mfrc522.uid.uidByte[i] < 0x10 ? " 0" : " "));
    content.concat(String(mfrc522.uid.uidByte[i], HEX));
  }

  Serial.println();
  Serial.print("UID value : ");
  Serial.println(content);

  //handle jika user sudah melakukan scan
  if(uidCard == content){
    Serial.println("Sudah Scan");
  }

  uidCard = content;
  // insertArduino(content);
  bool get_map = getMapping(id_arduino);

  if (get_map == true) {
    sendTransaksi(id_arduino, id_kantin, id_katering, content);
  }
  // getRfid(uidCard);
  return content;
}


/*
 function untuk get data dari mapping_arduino_katering, 
 untuk mencari id_kantin dan id_katering menggunakan id_arduino.
*/
bool getMapping(String id_arduino) {
  //cek apakah tersambung wifi
  if (WiFi.status() == WL_CONNECTED) {
    StaticJsonDocument<200> jsonDoc;
    jsonDoc["id_arduino"] = id_arduino;

    HTTPClient http;
    //post data id_arduino ke API (select mapping_arduino_katering)
    http.begin(wifiClient, API_MAPPING);
    http.addHeader("Content-Type", "application/json");
    String jsonString;
    serializeJson(jsonDoc, jsonString);
    int responseCode = http.POST(jsonString);

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
*/
bool sendTransaksi(String id_arduino, int id_kantin, int id_katering, String no_rfid) {
  //cek jika tersambung ke wifi
  if (WiFi.status() == WL_CONNECTED) {

    StaticJsonDocument<1024> jsonDoc;
    jsonDoc["id_arduino"] = String(id_arduino);
    jsonDoc["id_kantin"] = 1;
    jsonDoc["id_katering"] = 1;
    no_rfid.trim();
    // Masukkan string yang telah di-trim ke dalam JSON document
    jsonDoc["no_rfid"] = no_rfid.c_str();
    Serial.println("Kudune ngirim sih...");
    // Serial.println("kirim -> id arduino : " + id_arduino + " id kantin:" + String(id_kantin) + " id katering:" + String(id_katering) + " no rfid :" + no_rfid);


    HTTPClient http;
    //post data body json ke insert kupon_harian
    http.begin(wifiClient, API_TRANSACTION);
    http.addHeader("Content-Type", "application/json");
    // String postData = "{\"id_arduino\":\"" + String(id_arduino) + "\",\"id_kantin\":\"" + String(id_kantin) + "\",\"id_katering\":\"" + String(id_katering) + "\",\"id_rfid\":\"" + String(id_rfid) + "\"}";
    String jsonString;
    serializeJson(jsonDoc, jsonString);
    int responseCode = http.POST(jsonString);

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
      digitalWrite(led, HIGH);
      delay(1000);
      digitalWrite(led, LOW);  
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

//kode untuk menyimpan data ke flash memory
// void writeMemory(String konten) {
//   for (int i = 0; i < konten.length(); i++) {
//     EEPROM.write(int i, konten[i]);
//   }

//   EEPROM.write(konten.length(), '\0');
//   EEPROM.commit();
// }

//kode untuk membaca data dari flash memory
String readMemory() {
  String konten = "";
  for (int i = 0; i < 20; i++) {
    char c = EEPROM.read(i);
    if (c == '\0') break;
    konten += c;
  }
  return konten;
}
