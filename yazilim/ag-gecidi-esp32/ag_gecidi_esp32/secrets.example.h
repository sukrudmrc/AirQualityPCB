// secrets.example.h
// Bu dosyayi ayni klasore "secrets.h" adiyla kopyalayin ve degerleri doldurun.
// secrets.h .gitignore'a eklidir; gercek bilgiler depoya gonderilmez.
#pragma once

// Wi-Fi agi
#define WIFI_SSID          "wifi-adi"
#define WIFI_PASSWORD      "wifi-parolasi"

// ThingSpeak > Devices > MQTT ile olusturulan cihazin bilgileri
#define TS_MQTT_CLIENT_ID  "mqtt-istemci-kimligi"
#define TS_MQTT_USERNAME   "mqtt-kullanici-adi"
#define TS_MQTT_PASSWORD   "mqtt-parolasi"

// ThingSpeak kanal numarasi (yalnizca rakamlar, tirnak icinde)
#define TS_CHANNEL_ID      "0000000"
