#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h> // Biblioteca pentru salvarea definitiva a datelor
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME680.h>

#include <Firebase_ESP_Client.h>
#include "addons/TokenHelper.h"
#include "addons/RTDBHelper.h"

#define FIREBASE_HOST "https://licenta-smarthouse-default-rtdb.europe-west1.firebasedatabase.app" 
#define FIREBASE_AUTH "AIzaSyBXazQe0SunEO6EfkWYfMKM5gi1FLiEiaA"


FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;
unsigned long timpUltimaVerificare = 0;

// --- PINII PENTRU BANDA LED ---
const int PIN_ROSU = 12;
const int PIN_VERDE = 13;
const int PIN_ALBASTRU = 14;

// Obiectul pentru memoria non-volatilă
Preferences preferences;

// "Canalele" noastre Bluetooth (generate standard pentru industrie)
#define SERVICE_UUID           "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID    "beb5483e-36e1-4688-b7f5-ea07361b26a8"

// --- NOU: Declaram caracteristica global pentru a o putea apela de oriunde ---
BLECharacteristic *pCharacteristic;

bool bluetoothActivat = false;

bool setariNoiPrimite = false;
String reteaNoua = "";
String parolaNoua = "";

// Salvăm culorile curente pentru a le putea scrie în memorie la reset
int curentR = 0;
int curentG = 0;
int curentB = 0;

// Variabile pentru modul Ambient și setări globale ---
String modCurent = "solid"; 
int luminozitateGlobala = 100;
int vitezaAnimatie = 50;

// Variabile pentru a rula animația curcubeu fără delay
unsigned long ultimulPasCurcubeu = 0;
int pozitieCurcubeu = 0;

// Obiectul pentru senzorul BME680
Adafruit_BME680 bme;

// Variabilă pentru a nu trimite date non-stop, ci o dată la 5 secunde
unsigned long timpUltimaCitireSenzori = 0;

unsigned int ultimaActualizare = 0;
unsigned int lastHeartbeat = 0;

void aplicaCuloare(int r, int g, int b, int luminozitate) {
  // Aplicăm luminozitatea (matematică simplă: culoare * procentaj / 100)
  int rFinal = (r * luminozitate) / 100;
  int gFinal = (g * luminozitate) / 100;
  int bFinal = (b * luminozitate) / 100;

  analogWrite(PIN_ROSU, rFinal);
  analogWrite(PIN_VERDE, gFinal);
  analogWrite(PIN_ALBASTRU, bFinal);
}

// Transformă o poziție (0-255) într-o culoare din curcubeu
void ruleazaCadruCurcubeu(int pozitie, int luminozitate) {
  pozitie = 255 - pozitie;
  if (pozitie < 85) {
    aplicaCuloare(255 - pozitie * 3, 0, pozitie * 3, luminozitate);
  } else if (pozitie < 170) {
    pozitie -= 85;
    aplicaCuloare(0, pozitie * 3, 255 - pozitie * 3, luminozitate);
  } else {
    pozitie -= 170;
    aplicaCuloare(pozitie * 3, 255 - pozitie * 3, 0, luminozitate);
  }
}

// Aici prindem mesajul primit prin Bluetooth
class MyCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) {
      String valoare = pCharacteristic->getValue().c_str();

      if (valoare.length() > 0) {
        Serial.println("\n[BLE] Mesaj primit: ");
        Serial.println(valoare);
        
        // Așteptăm formatul "NumeRetea,Parola"
        int indexVirgula = valoare.indexOf(',');
        if (indexVirgula != -1) {
          reteaNoua = valoare.substring(0, indexVirgula);
          parolaNoua = valoare.substring(indexVirgula + 1);
          
          reteaNoua.trim(); // Eliminăm eventualele spații goale în plus
          parolaNoua.trim();
          
          setariNoiPrimite = true;
        } else {
          Serial.println("[BLE] Format incorect. Foloseste: SSID,Parola");
        }
      }
    }
};

void conectareWiFi(String ssid, String pass) {
  Serial.print("\n[WiFi] Incercare conectare la: ");
  Serial.println(ssid);
  WiFi.begin(ssid.c_str(), pass.c_str());
  
  int incercari = 0;
  // Așteptăm maxim 10 secunde (20 x 0.5s)
  while (WiFi.status() != WL_CONNECTED && incercari < 20) {
    delay(500);
    Serial.print(".");
    incercari++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\n[WiFi] CONECTAT CU SUCCES!");
    Serial.print("[WiFi] Adresa IP: ");
    Serial.println(WiFi.localIP());
    
    // Salvăm noile date permanent în memoria ESP32
    preferences.begin("setari-wifi", false);
    preferences.putString("ssid", ssid);
    preferences.putString("parola", pass);
    preferences.end();
    
    Serial.println("[SISTEM] Setarile WiFi au fost salvate definitiv!");

    // --- NOU: Trimitem mesajul OK catre telefon ---
    if (pCharacteristic != nullptr) {
        Serial.println("[BLE] Trimit confirmarea WIFI_OK catre telefon...");
        pCharacteristic->setValue("WIFI_OK");
        pCharacteristic->notify();
        delay(1000); // Asteptam o secunda ca telefonul sa apuce sa citeasca mesajul
    }

    // Oprim Bluetooth-ul pentru a salva energie, ne-am făcut treaba cu el
    BLEDevice::deinit(true); 
    Serial.println("[BLE] Bluetooth oprit pentru economisire energie.");

    bluetoothActivat = false;

    // --- PORNIRE FIREBASE DUPĂ CONECTAREA LA INTERNET ---
    Serial.println("[FIREBASE] Ma conectez la baza de date...");
    config.api_key = FIREBASE_AUTH;
    config.database_url = FIREBASE_HOST;
    Firebase.signUp(&config, &auth, "", "");
    Firebase.begin(&config, &auth);
    Firebase.reconnectWiFi(true);
    Serial.println("[FIREBASE] Initializat cu succes!");
  } else {
    Serial.println("\n[WiFi] Eroare: Nu m-am putut conecta. Verifica parola!");
    // Optional: Aici am putea trimite un "WIFI_FAIL" catre telefon pe viitor
  }
  
}


void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println("\n\n--- Pornire Sistem Smart Home ---");

  // --- INIȚIALIZARE PINI BANDA LED ---
  pinMode(PIN_ROSU, OUTPUT);
  pinMode(PIN_VERDE, OUTPUT);
  pinMode(PIN_ALBASTRU, OUTPUT);
  // Stingem banda la pornire
  analogWrite(PIN_ROSU, 0);
  analogWrite(PIN_VERDE, 0);
  analogWrite(PIN_ALBASTRU, 0);

  preferences.begin("setari-wifi", false);
  String ssidSalvat = preferences.getString("ssid", "");
  String parolaSalvata = preferences.getString("parola", "");
  preferences.end();

  // Inițializăm senzorul BME680 pe adresa I2C 0x76 (specifică plăcuțelor Pimoroni)
  if (!bme.begin(0x76)) {
    Serial.println("[EROARE] Nu gasesc senzorul BME680! Verifica firele SDA si SCL.");
  } else {
    Serial.println("[SISTEM] Senzor BME680 initializat cu succes!");
    // Setăm precizia (Oversampling)
    bme.setTemperatureOversampling(BME680_OS_8X);
    bme.setHumidityOversampling(BME680_OS_2X);
    bme.setPressureOversampling(BME680_OS_4X);
    bme.setIIRFilterSize(BME680_FILTER_SIZE_3);
    // Setăm "încălzitorul" intern pentru a citi gazele/calitatea aerului
    bme.setGasHeater(320, 150); // 320 grade Celsius timp de 150 milisecunde
  }

  if (ssidSalvat != "NO NETWORK" && ssidSalvat != "") {
    Serial.println("[SISTEM] Am gasit o retea salvata in memorie.");
    conectareWiFi(ssidSalvat, parolaSalvata);
  } else {
    Serial.println("[SISTEM] Nu exista retea salvata. Sar peste conectare Wi-Fi.");
  }

  // Dacă Wi-Fi a picat sau am sărit peste el, pornim Bluetooth curat
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("[SISTEM] Pornesc modulul Bluetooth pentru configurare...");
    
    BLEDevice::init("SmartHome-Licenta");
    BLEServer *pServer = BLEDevice::createServer();
    BLEService *pService = pServer->createService(SERVICE_UUID);
    
    pCharacteristic = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID,
                                         BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY
                                       );

    pCharacteristic->addDescriptor(new BLE2902());
    pCharacteristic->setCallbacks(new MyCallbacks());
    pService->start();
    
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(SERVICE_UUID);
    pAdvertising->setScanResponse(true);
    BLEDevice::startAdvertising();
    
    Serial.println("[BLE] Bluetooth activ! Cauta dispozitivul cu telefonul.");
  }
}



void loop() {
  // --- 1. HEARTBEAT & STATUS (O singură dată la 10 secunde) ---
  if (millis() - lastHeartbeat > 10000) {
    lastHeartbeat = millis();
    if (WiFi.status() == WL_CONNECTED && Firebase.ready()) {
      // Trimitem ambele informații deodată pentru a nu aglomera SSL-ul
      Firebase.RTDB.setTimestamp(&fbdo, "/dispozitiv/last_seen");
      Firebase.RTDB.setString(&fbdo, "/dispozitiv/status", "online");
      Serial.println("[SISTEM] Status & Heartbeat trimis.");
    }
  }

  // --- 2. LOGICA DE CITIRE CULORI (Mai eficientă) ---
  // --- 2. LOGICA DE CITIRE DATE DIN FIREBASE (La fiecare 500ms) ---
  if (WiFi.status() == WL_CONNECTED && Firebase.ready() && (millis() - timpUltimaVerificare > 500)) {
    timpUltimaVerificare = millis();

    if (Firebase.RTDB.getJSON(&fbdo, "/banda_led")) {
      FirebaseJson json = fbdo.jsonObject();
      FirebaseJsonData data;

      // Citim noile setări globale
      if(json.get(data, "mod")) modCurent = data.stringValue;
      if(json.get(data, "luminozitate")) luminozitateGlobala = data.intValue;
      if(json.get(data, "viteza")) vitezaAnimatie = data.intValue;

      // Citim culorile brute
      if(json.get(data, "r")) curentR = data.intValue;
      if(json.get(data, "g")) curentG = data.intValue;
      if(json.get(data, "b")) curentB = data.intValue;

      // Dacă suntem pe modul "solid", aplicăm culoarea imediat cum am primit-o
      if (modCurent == "solid") {
        aplicaCuloare(curentR, curentG, curentB, luminozitateGlobala);
      }
    }
  }

  // --- NOU: LOGICA PENTRU ANIMAȚIA AMBIENT (Rulează la foc continuu) ---
  if (modCurent == "ambient") {
    // Calculăm cât de repede se mișcă curcubeul (Delay între cadre)
    // Viteza 100% -> 5ms (foarte rapid)
    // Viteza 0% -> 100ms (foarte lent)
    int intervalAnimatie = map(vitezaAnimatie, 0, 100, 100, 5);

    // Dacă a trecut suficient timp de la ultimul cadru, trecem la următoarea culoare
    if (millis() - ultimulPasCurcubeu > intervalAnimatie) {
      ultimulPasCurcubeu = millis();
      
      ruleazaCadruCurcubeu(pozitieCurcubeu, luminozitateGlobala);
      
      // Avansăm pe roata culorilor (0-255)
      pozitieCurcubeu++;
      if (pozitieCurcubeu > 255) {
        pozitieCurcubeu = 0;
      }
    }
  }

  // --- 3. GESTIONARE BLUETOOTH ---
  if (setariNoiPrimite) {
    setariNoiPrimite = false;
    Serial.println("\n[SISTEM] Se aplica noile setari...");
    conectareWiFi(reteaNoua, parolaNoua);
  }

  static unsigned long timpUltimaVerificareComenzi = 0;

  if (WiFi.status() == WL_CONNECTED && Firebase.ready() && (millis() - timpUltimaVerificareComenzi > 1000)) {
    timpUltimaVerificareComenzi = millis();

    if (Firebase.RTDB.getBool(&fbdo, "/dispozitiv/comenzi/activare_ble")) {
      bool cereBle = fbdo.boolData();
      
      if (cereBle == true) {
        Serial.println("\n[FIREBASE] Comanda primita! Trecem in modul de Configurare...");
        
        // 1. Resetăm valoarea pe server ca să nu râmână blocat
        Firebase.RTDB.setBool(&fbdo, "/dispozitiv/comenzi/activare_ble", false);
        
        // 2. Ștergem datele Wi-Fi vechi din memoria ESP-ului
        preferences.begin("setari-wifi", false);
        preferences.putString("ssid", "NO NETWORK"); // Punem un text fals ca să pice intenționat logarea
        preferences.end();

        preferences.begin("stare-led", false);
        preferences.putInt("r", curentR);
        preferences.putInt("g", curentG);
        preferences.putInt("b", curentB);
        preferences.end();
        
        Serial.println("[SISTEM] Reteaua a fost stearsa! Placa se va restarta in 3 secunde...");
        delay(3000);
        
        // 3. Repornim complet placa (Hardware Reset)
        ESP.restart();
      }
    }
  }

  // 4. --TRIMITERE DATE AER + TEMPERATURA CATRE FIREBASE --
  if (WiFi.status() == WL_CONNECTED && Firebase.ready() && (millis() - timpUltimaCitireSenzori > 5000)) {
    timpUltimaCitireSenzori = millis();

    // Spunem senzorului să facă o citire proaspătă
    if (bme.performReading()) {
      
      float temperatura = bme.temperature - 1.5; // Corecție de calibrare (am observat că e cu 1.5 grade prea mare)
      float umiditate = bme.humidity;
      
      // Calitatea aerului se bazează pe "Gas Resistance" (Rezistența gazului).
      // Cu cât e mai mare, cu atât aerul e mai curat (nu sunt gaze nocive care să conducă electricitatea).
      // Un aer curat are de obicei peste 50.000 Ohmi. Unul poluat scade sub 10.000 Ohmi.
      // Transformăm în kilo-ohmi
      int rezistentaKOhmi = bme.gas_resistance / 1000;
      
      // Mapare INVERSATĂ: 200 kOhm (Aer Curat) = 0 AQI | 10 kOhm (Glo/Fum) = 500 AQI
      int scorCalitateAer = map(rezistentaKOhmi, 200, 10, 0, 500);
      
      // Ne asigurăm că valoarea nu depășește intervalul 0 - 500
      scorCalitateAer = constrain(scorCalitateAer, 0, 500);

      // Afișăm în consolă să vedem noi ce se întâmplă
      Serial.print("Temp: "); Serial.print(temperatura); Serial.print(" *C | ");
      Serial.print("Umiditate: "); Serial.print(umiditate); Serial.print(" % | ");
      Serial.print("Calitate Aer: "); Serial.print(scorCalitateAer); Serial.println(" %");

      // Trimitem totul către Firebase într-un singur pachet eficient
      FirebaseJson jsonSenzori;
      jsonSenzori.set("temperatura", temperatura);
      jsonSenzori.set("umiditate", umiditate);
      jsonSenzori.set("calitate_aer", scorCalitateAer);

      Firebase.RTDB.setJSON(&fbdo, "/senzori", &jsonSenzori);
    } else {
      Serial.println("[EROARE] Nu am putut citi de la senzorul BME680.");
    }
  }

  delay(1); // Un delay minim pentru stabilitatea sistemului de operare (FreeRTOS)
}