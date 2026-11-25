#include <HardwareSerial.h>
#include <WiFi.h>
#include <PubSubClient.h>
//=============================================
#define RX2_PIN 16  // UART2 RX Pin
#define TX2_PIN 17  // UART2 TX Pin

#define M0_PIN 4    // M0 Pin
#define M1_PIN 5    // M1 Pin

HardwareSerial mySerial(2);
//====================WiFi / MQTT bilgileri=================================
// Kimlik bilgileri depoya konmaz. secrets.example.h dosyasini secrets.h
// adiyla kopyalayip kendi degerlerinizi yazin (secrets.h .gitignore'dadir).
#include "secrets.h"

const char *ssid = WIFI_SSID;
const char *pass = WIFI_PASSWORD;

const char* mqtt_server = "mqtt3.thingspeak.com";
const int mqtt_port = 1883;
const char* mqttClientID = TS_MQTT_CLIENT_ID;
const char* mqttUsername = TS_MQTT_USERNAME;
const char* mqttPassword = TS_MQTT_PASSWORD;

const char* topic = "channels/" TS_CHANNEL_ID "/publish";

//====================VARIABLES============================================

const long interval= 60000; //cycle time
long previousMillis=0;
//Lora Destination
byte destinationHigh = 0x00;  // Adres 3'ün yüksek baytı
byte destinationLow = 0x01;   // Adres 3'ün düşük baytı
byte channel = 0x02;
// Sensor Buffers
float pm25_buff;
float pm10_buff;
float temp_buff;
float humidity_buff;
float co_buff;

uint8_t RX_Buff[13];

bool SensorFlag=0;

//=======================================================================

WiFiClient espClient;
PubSubClient client(espClient);

//==========================FUNCTIONS-PROTOTYES===========================
//Lora Functions
void WakeUP_command(void);
void readSensorPacket(void);
void USBtoE220(void);

//WiFi & MQTT Functions
void contoWIFI(void);
void connectMQTT(void);
void sendToThingSpeak(void);

//======================================================================
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  mySerial.begin(9600, SERIAL_8N1, RX2_PIN, TX2_PIN);

  Serial.print("System Launched\n");
  contoWIFI();
  delay(1000);
  client.setServer(mqtt_server,mqtt_port);
  pinMode(5, OUTPUT);  // GPIO 5'i çıkış olarak ayarla
  digitalWrite(5, LOW);
  pinMode(18, OUTPUT);  // GPIO 5'i çıkış olarak ayarla
  digitalWrite(18, LOW); 
}

void loop() {
  // put your main code here, to run repeatedly:
  if(WiFi.status() != WL_CONNECTED){
    Serial.print("Connection lost...");
    contoWIFI();
    }

    client.loop();
    if (!client.connected()) {
    connectMQTT();
  }

  if(millis()-previousMillis >= interval){
    previousMillis = millis();
    WakeUP_command();
  }

  if (Serial.available()) {
    USBtoE220();
  }

  if(mySerial.available()){
    readSensorPacket();
    delay(500);
      if(SensorFlag){
        client.loop();
        if (!client.connected()) {
         connectMQTT();
        }
        sendToThingSpeak();
        SensorFlag=0;
        previousMillis = millis();
      }
  }
}

//========================FUNCTIONS====================================
void WakeUP_command(void){
  // To get the Sensor Data Sends a Wakeup call,Any byte or bit would interrupt and wake the system up
  // The Transiver uses UART interrupr so whenever a signal send to a Lora System would wakeup   
    String WakeupMes= "n";
    mySerial.write(destinationHigh);
    mySerial.write(destinationLow);
    mySerial.write(channel);
    mySerial.print(WakeupMes);

    Serial.print("Uyandırılıyor: ");
    Serial.print(WakeupMes);
    Serial.print("\n");
}

void readSensorPacket(void){
  int recieved = 0;
  
  while(recieved < 13 ){
    if(mySerial.available()){
      RX_Buff[recieved++] = mySerial.read();
      if(recieved == 1 && RX_Buff[0] != 0xAA){//Wait For start byte 0xAA
        recieved = 0;  // senkron kaydı, baştan başla
      }
    }
  }

  uint8_t CheckSum=0; // 11th Byte CheckSum byte 
  for(int i=1;i <= 10 ;i++){
    CheckSum += RX_Buff[i];
    }

  if(RX_Buff[0] == 0xAA && RX_Buff[12] == 0xAB && RX_Buff[11] == CheckSum){//Does all conditions are true
    pm25_buff=     (RX_Buff[2] << 8 | RX_Buff[1]);
    pm10_buff=     (RX_Buff[4] << 8 | RX_Buff[3]);
    co_buff  =     (RX_Buff[6] << 8 | RX_Buff[5]);
    temp_buff=     (RX_Buff[8] << 8 | RX_Buff[7]);
    humidity_buff= (RX_Buff[10] << 8 | RX_Buff[9]);
    // /10 is for creating a float data
    // Lora messages are int8 with highs and lows
    // so instead of send 22.5 it sends 225

  pm25_buff /=10;
  pm10_buff /=10;
  co_buff /=10;
  temp_buff /=10;
  humidity_buff /=10;

  Serial.println(pm25_buff);
  Serial.println(pm10_buff);
  Serial.println(co_buff);
  Serial.println(temp_buff);
  Serial.println(humidity_buff);

  
  SensorFlag=1;
  } else {
    Serial.println("\nNo Resonable Data");
    WakeUP_command(); // Get a new data again 
  }
}

void USBtoE220(void){ // to be able to wake-up manually
    mySerial.write(destinationHigh);
    mySerial.write(destinationLow);
    mySerial.write(channel);
    mySerial.write(Serial.read());
}

void contoWIFI(void){ // Connect to WiFi
  Serial.print("Connecting to Wifi\n");
  WiFi.begin(ssid,pass);

  int retries=0;
  while(WiFi.status() != WL_CONNECTED){
    delay(5000);
    retries++;
    Serial.print(".  \n");
    if(retries>20 && WiFi.status() != WL_CONNECTED){
      Serial.print("Connection Failed\n");
      Serial.print("...\n");
      Serial.print("Restarting");
      ESP.restart();
    }
  }

  Serial.print("Connection Succesful\n");
  Serial.print("IP:");
  Serial.println(WiFi.localIP());
}

void connectMQTT() { // Connect to MQTT
  while (!client.connected()) {
    Serial.print("MQTT bağlanıyor...");
    if (client.connect(mqttClientID, mqttUsername, mqttPassword)) {
      Serial.println(" OK");
    } else {
      Serial.print(" Hata=");
      Serial.println(client.state()); // -4 auth hatası, -2 timeout vb.
      delay(2000);
    }
  }
}

void sendToThingSpeak(void) {
    String payload = "field1=" + String(pm25_buff,2) +
                   "&field2=" + String(pm10_buff,2) +
                   "&field3=" + String(co_buff,2) +
                   "&field4=" + String(temp_buff,2) +
                   "&field5=" + String(humidity_buff,2) +
                   "&status=MQTTPUBLISH";
  Serial.println("Gönderiliyor → " + payload);
  client.publish(topic, payload.c_str());
}