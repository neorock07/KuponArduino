//rename serial, helpful when using multiple serial connections
#define MySerial Serial

//wifi library, only used to turn it off since we are using an ethernet adapter
//library is included esp32 board manager package by Expressif Systems
// #include <WiFi.h>
#include <ArduinoJson.h>
#include <ArduinoJson.hpp>
#include <HTTPClient.h>
#include <SPI.h>
//library RFID
#include <MFRC522.h>
#include <Wire.h>
#include <EthernetESP32.h>  //library modifikasi dari Ethernet.h
//esp library ETH does not work with the w5500
byte mac[] = { 0x74, 0x69, 0x69, 0x2D, 0x32, 0x33 };  // ethernet mac address - harus unik <perbarui untuk setiap kode esp>

String readString;                  //var for url requested, this allows us to serve different pages based on the url
String API_HOST = "192.168.137.1";  // Host API
String API_MAPPING = "/ProyekKupon/api/mapping_arduino_katering/select";
String API_RFID = "/ProyekKupon/api/rfid/select";
String API_TRANSACTION = "/ProyekKupon/api/kupon_harian/insert";
String API_ARDUINO = "/ProyekKupon/api/arduino/insert";

// extern SPIClass SPI2;
//init Ethernet
EthernetServer server(80);  //set HTTP server port
EthernetClient client;

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
//variabel untuk id unik alat
String id_arduino = "GR 45 T";
// Pin GPIO yang terhubung ke LED
const int ledPin = 2;

//konfigurasi scs w5500
#define W5500_CS 21

//konfigurasi pin untuk rfid
#define RST_PIN 22
#define MFRC522_CS 5
//inisiasi rfid
MFRC522 mfrc522(MFRC522_CS, RST_PIN);
bool berhasil = false;
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


int count = 0;

void setup() {
  MySerial.begin(9600);
  //untuk loading serial dan koneksi eth
  delay(2000);

  // Mengatur pin sebagai output
  pinMode(ledPin, OUTPUT);
  pinMode(MFRC522_CS, OUTPUT);
  pinMode(W5500_CS, OUTPUT);

  // Nonaktifkan kedua perangkat awalnya
  // digitalWrite(MFRC522_CS, HIGH);  // Nonaktifkan MFRC522
  // digitalWrite(W5500_CS, HIGH);    // Nonaktifkan W5500

  // Inisialisasi SPI
  SPI.begin(18, 19, 23, MFRC522_CS);  // Inisialisasi SPI secara umum
  // SPI2.begin(14,12,13,5);
  // Konek ke ethernet
  // digitalWrite(W5500_CS, LOW);     // Aktifkan W5500
  Ethernet.init(W5500_CS);  // Inisialisasi Ethernet
  bool isConnected = connect_ethernet();
  // digitalWrite(W5500_CS, HIGH);    // Nonaktifkan W5500 setelah inisialisasi

  if (isConnected) {
    MySerial.println("===TERHUBUNG KE ETHERNET===");
    MySerial.println("Dekatkan kartu ke reader!");
  } else {
    MySerial.println("===TIDAK TERHUBUNG ETHERNET===");
  }

  // Inisialisasi MFRC522
  // digitalWrite(MFRC522_CS, LOW);   // Aktifkan MFRC522
  // SPI.begin(14, 12, 13, MFRC522_CS);           // Konfigurasi pin SPI untuk MFRC522
  mfrc522.PCD_Init();  // Inisialisasi MFRC522
  // digitalWrite(MFRC522_CS, HIGH);  // Nonaktifkan MFRC522 setelah inisialisasi
}

void loop() {
  Ethernet.maintain();
  Serial.println("belum ada kartu");
  Serial.print("Status ETH : ");
  Serial.println(Ethernet.linkStatus());
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
  function untuk connect ke ethernet dan mendapatkan IP address dari router/laptop/switch
*/
bool connect_ethernet() {
  MySerial.println("Begin Setup...");
  //start the Ethernet connection and the server:
  // SPI.begin();
  // Ethernet.init(W5500_CS);  //set the CS pin, required for ESP32 as the arduino default is different
  MySerial.println("====GETTING IP ADDRESS===");
  Ethernet.begin(mac);
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
    //tampilkan ip address ESP32
    MySerial.print(F("Server is at "));
    MySerial.println(Ethernet.localIP());

    MySerial.println("Setup Complete.");
    MySerial.println("");
  }
  return berhasil;
}


/*
  function untuk membaca UID card
  Args:
    void
  returns:
    String content : mengembalikan string UID kartu  
*/
String readRfid() {
  // digitalWrite(W5500_CS, HIGH);  // Nonaktifkan W5500
  // digitalWrite(MFRC522_CS, LOW);
  //deteksi apakah ada kartu atau select jika ada kartu
  if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
    return "";
  }
  // Saat ingin menggunakan Ethernet:
  // digitalWrite(MFRC522_CS, HIGH);  // Nonaktifkan MFRC522

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
    // digitalWrite(W5500_CS, LOW);  // Aktifkan W5500
    // Proses Ethernet
    bool get_map = getMapping(id_arduino);

    if (get_map == true) {
      sendTransaksi(id_arduino, id_kantin, id_katering, content);
      Serial.println("===TERKIRIM===");
      Serial.println("id_Arduino : " + String(id_arduino) + "id_kantin : " + String(id_kantin) + "id_katering : " + String(id_katering) + "RFID : " + String(content));
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
      }

      client.stop();  // Tutup koneksi
      return true;
    } else {
      MySerial.println("Connection to server failed");
      return false;
    }
  }

  delay(1000);
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
  if (Ethernet.linkStatus() == LinkON) {
    if (client.connect(API_HOST.c_str(), 80)) {
      MySerial.println("Connected to server");
      // Trim left pada no_rfid (menghapus spasi di depan)
      while (no_rfid.length() > 0 && no_rfid[0] == ' ') {
        no_rfid.remove(0, 1);  // hapus karakter pertama jika spasi
      }

      Serial.println("RFID SEND : " + no_rfid);
      Serial.println("ID ARDUINO SEND : " + id_arduino);
      Serial.println("ID KANTIN SEND : " + String(id_kantin));
      Serial.println("ID KATERING SEND : " + String(id_katering));

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
      uint8_t* msg = (uint8_t*)malloc(size + 1);
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
