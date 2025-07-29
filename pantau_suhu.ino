#define PAKAI_WEBSOCKET

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <DHT.h>
#include "mqtt_secrets.h"

// Konfigurasi WiFi
const char wifi_ssid[] = "C42";
const char wifi_pass[] = "0806040200";

// Konfigurasi ThingSpeak MQTT
const char* mqttServer = "mqtt3.thingspeak.com";
const int mqttPort = 1883;
const char* mqttUser = SECRET_MQTT_USERNAME; // User ID ThingSpeak
const char* mqttPassword = SECRET_MQTT_PASSWORD; // Password MQTT
const char* clientId = SECRET_MQTT_CLIENT_ID; // Client ID
const char* topic = "channels/3015633/publish"; // Ganti CHANNEL_ID dengan ID channel Anda

// Konfigurasi DHT
//#define DHT_PIN D2     // Pin yang terhubung ke DHT22
#define DHT_PIN D5
#define DHT_TYP DHT22 // Tipe sensor DHT22

DHT dht(DHT_PIN, DHT_TYP);
float dht_hum, dht_tem;

WiFiClient client;
// WiFiClientSecure client;
PubSubClient mqtt(client);

uint32_t last_wifi_check = 0;
uint32_t last_conn_check = 0;
uint32_t last_time = 0;
uint32_t last_kirim = 0;

void setup() {
  Serial.begin(115200);
  dht.begin();

  Serial.print(F("Connecting to "));
  Serial.println(wifi_ssid);
  WiFi.setAutoReconnect(true);
  WiFi.persistent(true);
  WiFi.begin(wifi_ssid, wifi_pass);
  wifi_connect_process();

  // client.setInsecure();
  mqtt.setServer(mqttServer, mqttPort);

  Serial.println("Mulai");
}

void loop() {
  // Menghubungkan ulang ke wifi jika terputus
  while (millis() - last_wifi_check >= 5000) {
    last_wifi_check = millis();
    if (WiFi.status() != WL_CONNECTED) {
      Serial.println(F("Menghubungkan ulang ke WiFi..."));
      wifi_connect_process();
    }
  }

  // Cek koneksi MQTT
  if (!mqtt.connected()) {
    mqtt_reconnect();
  }
  mqtt.loop();

  while (millis() - last_time >= 2500) {
    // Baca data dari sensor DHT22
    dht_hum = dht.readHumidity();
    dht_tem = dht.readTemperature(); // Baca suhu dalam Celsius
    
    // Periksa jika pembacaan gagal
    if (isnan(dht_hum) || isnan(dht_tem)) {
      Serial.println("Gagal membaca dari sensor DHT!");
      return;
    }

    Serial.print("Suhu: ");
    Serial.print(dht_tem);
    Serial.print(" °C, Kelembaban: ");
    Serial.print(dht_hum);
    Serial.println(" %");

    last_time = millis();
  }

  // Tunggu 20 detik sebelum pengiriman berikutnya
  // (ThingSpeak free account limit: 15 detik)
  while (millis() - last_kirim >= 20000) {
    // Format data untuk dikirim ke ThingSpeak
    String data = "field1=" + String(dht_tem) + "&field2=" + String(dht_hum);
    
    yield();
    // Publikasikan data ke ThingSpeak
    if (mqtt.publish(topic, data.c_str())) {
      Serial.println("Data terkirim ke ThingSpeak");
    } else {
      Serial.println("Gagal mengirim data");
    }

    last_kirim = millis();
  }
}

void wifi_connect_process() {
  while (1) {
    if (millis() - last_conn_check <= 500) {
      /*esp_*/yield();
      continue;
    }
    if (WiFi.status() == WL_CONNECTED) {
      break;
    }

    Serial.print(".");
    last_conn_check = millis();
  }

  Serial.println("");
  Serial.println(F("WiFi terhubung"));
  Serial.println(F("Alamat IP: "));
  Serial.println(WiFi.localIP());
}

void mqtt_reconnect() {
  while (!mqtt.connected()) {
    Serial.print("Mencoba koneksi MQTT...");
    
    if (mqtt.connect(clientId, mqttUser, mqttPassword)) {
      Serial.println("terhubung");
      break;
    }

    Serial.print("gagal, rc=");
    Serial.print(mqtt.state());
    Serial.println(" coba lagi dalam 5 detik");

    // delay(5000);
    last_conn_check = millis();
    while (millis() - last_conn_check <= 5000) {
      yield();
    }
  }
}
