#include <Arduino.h>
#include <PubSubClient.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include "DHT.h"
#define DHTPIN 21    
#define DHTTYPE DHT22 
DHT dht(DHTPIN, DHTTYPE);
#define ldrPin 34 
const float gama = 0.7;
const float rl10 = 50;
bool status_air = false;
bool status_lampu = false;
bool status_Kipas = false;

float kecerahan;
float t, h;

//wifi data
const char *ssid = ".";
const char *password = "123456789";

// Mosquitto Broker
const char *mqtt_server = "test.mosquitto.org";
const int mqtt_port = 1883;
long now = millis();
long lastMsg = 0;

//mqtt
WiFiClient espClient;
PubSubClient client(espClient);

//relay pin
int relay_air = 33; // air
int relay_lampu = 32; // lampu
int relay_Kipas = 27; // kipas

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Attempt to connect
    if (client.connect("ESP32Client")) {
      Serial.println("Connected");
      // Once connected, publish an announcement...
      client.publish("sensor/cahaya", String(kecerahan).c_str());
     client.publish("sensor/suhu", String(t).c_str());
     client.publish("sensor/kelembaban", String(h).c_str());
     client.publish("Kipas", String(status_Kipas).c_str());
     client.publish("lampu", String(status_lampu).c_str());
     client.publish("air", String(status_air).c_str());
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
      bacaSensor();
      rules();
    }
  }
}


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
  pinMode(relay_Kipas, OUTPUT);
  pinMode(relay_lampu, OUTPUT);
  pinMode(relay_air, OUTPUT);
  digitalWrite(relay_Kipas, HIGH);
  digitalWrite(relay_lampu, HIGH);
  digitalWrite(relay_air, HIGH);
  dht.begin();
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  
  while (!client.connected()) 
   {
     Serial.println("Connecting to MQTT...");
     
     if (client.connect("ESP32Client")) 
     {
       client.publish("outTopic", "Connected!");
       Serial.println("connected");  
     } 
     else 
     {
       Serial.print("failed with state ");
       Serial.print(client.state());
     }
   }
   client.publish("sensor/cahaya", String(kecerahan).c_str());
   client.publish("sensor/suhu", String(t).c_str());
   client.publish("sensor/kelembaban", String(h).c_str()); 
   client.publish("Kipas", String(status_Kipas).c_str());
   client.publish("lampu", String(status_lampu).c_str());
   client.publish("air", String(status_air).c_str());
}

void loop() {
  // put your main code here, to run repeatedly:
  if (!client.connected()){
    reconnect();
  }
  client.loop();

  unsigned long now = millis();\
  if(now - lastMsg > 5000){
   client.publish("sensor/cahaya", String(kecerahan).c_str());
   client.publish("sensor/suhu", String(t).c_str());
   client.publish("sensor/kelembaban", String(h).c_str());
   client.publish("Kipas", String(status_Kipas).c_str());
   client.publish("lampu", String(status_lampu).c_str());
   client.publish("air", String(status_air).c_str());
  }
  bacaSensor();
  rules();
}

void suhu(){
  t = dht.readTemperature();
  h = dht.readHumidity();
}

void cahaya(){
  int nilaiLDR = analogRead(ldrPin);
  nilaiLDR = map(nilaiLDR, 4095, 0, 1024, 0); //mengubah nilai pembacaan sensor LDR dari nilai ADC arduino menjadi nilai ADC ESP32
  float voltase = nilaiLDR / 1024.*5;
  float resistansi = 2000 * voltase / (1-voltase/5);
  kecerahan = pow(rl10*1e3*pow(10,gama)/resistansi,(1/gama));
}

void bacaSensor(){
  suhu();
  Serial.print("Humidity: ");
  Serial.print(h);
  Serial.print(" %\t");
  Serial.print("Temperature: ");
  Serial.print(t);
  Serial.println(" *C ");

  cahaya();  
  Serial.print("Kecerahan = ");
  Serial.println(kecerahan);
}



void rules(){
  if(int(kecerahan)< 100){
    Serial.println("Redup");
  if(t <= 15 && h <= 79){
    digitalWrite(relay_lampu, LOW);
    digitalWrite(relay_air, LOW);
    digitalWrite(relay_Kipas, HIGH);
    Serial.println("Lampu dan air nyala");
    status_lampu = true;
    status_air = true;
  } else if(t <= 15 && h >= 96){
    digitalWrite(relay_lampu, LOW);
    digitalWrite(relay_Kipas, LOW);
    digitalWrite(relay_air, HIGH);
    status_lampu = true;
    status_Kipas = true;
    Serial.println("Lampu dan Kipas nyala");
  }else if(t >= 31 && h <= 79){
    digitalWrite(relay_Kipas, LOW);
    digitalWrite(relay_air, LOW);
    digitalWrite(relay_lampu, HIGH);
    status_Kipas = true;
    status_air = true;
    Serial.println("Kipas dan air nyala");
  }else if(t >= 31 && h >= 96){
    digitalWrite(relay_Kipas, LOW);
    digitalWrite(relay_lampu, LOW);
    digitalWrite(relay_air, HIGH);
    status_lampu = true;
    status_Kipas = true;
    Serial.println("lampu dan Kipas nyala");
  }else if(16>=t<=30 && 80>=h<=95){
    digitalWrite(relay_Kipas, HIGH);
    digitalWrite(relay_lampu, HIGH);
    digitalWrite(relay_air, HIGH);
    status_lampu = false;
    status_air = false;
    status_Kipas = false;
    Serial.println("Lampu, Kipas, air mati");
  }else if(t <= 15 && 80>=h<=95){
    digitalWrite(relay_lampu, LOW);
    digitalWrite(relay_air, HIGH);
    digitalWrite(relay_Kipas, HIGH);
    status_lampu = true;
    Serial.println("lampu nyala");
  }else if(16>=t<=30 && h <= 79){
    digitalWrite(relay_air, LOW);
    digitalWrite(relay_Kipas, HIGH);
    digitalWrite(relay_lampu, HIGH);
    status_air = true;
    Serial.println("Air nyala");
  }else if(16>=t<=30 && h >= 96){
    digitalWrite(relay_Kipas, LOW);
    digitalWrite(relay_lampu, LOW);
    digitalWrite(relay_air, HIGH);
    status_lampu = true;
    status_Kipas = true;
    Serial.println("lampu dan Kipas nyala");
  }else if(t >= 31 && 80>=h<=95){
    digitalWrite(relay_Kipas, LOW);
    digitalWrite(relay_lampu, HIGH);
    digitalWrite(relay_air, HIGH);
    status_Kipas = true;
    Serial.println("Kipas nyala");
  }  
}else if(int(kecerahan)>=100){
  Serial.println("Cerah");
  if(t <= 15 && h <= 79){
    digitalWrite(relay_lampu, LOW);
    digitalWrite(relay_air, LOW);
    digitalWrite(relay_Kipas, HIGH);
    status_lampu = true;
    status_air = true;
    Serial.println("Lampu dan air nyala");
  } else if(t <= 15 && h >= 96){
    digitalWrite(relay_lampu, LOW);
    digitalWrite(relay_Kipas, LOW);
    digitalWrite(relay_air, HIGH);
    status_lampu = true;
    status_Kipas = true;
    Serial.println("Lampu dan Kipas nyala");
  }else if(t >= 31 && h <= 79){
    digitalWrite(relay_Kipas, LOW);
    digitalWrite(relay_air, LOW);
    digitalWrite(relay_lampu, HIGH);
    status_Kipas = true;
    status_air = true;
    Serial.println("Kipas dan air nyala");
  }else if(t >= 31 && h >= 96){
    digitalWrite(relay_Kipas, LOW);
    digitalWrite(relay_lampu, LOW);
    digitalWrite(relay_air, HIGH);
    status_lampu = true;
    status_Kipas = true;
    Serial.println("lampu dan Kipas nyala");
  }else if(16>=t<=30 && 80>=h<=95){
    digitalWrite(relay_Kipas, HIGH);
    digitalWrite(relay_lampu, HIGH);
    digitalWrite(relay_air, HIGH);
    status_lampu = false;
    status_air = false;
    status_Kipas = false;
    Serial.println("Lampu, Kipas, air mati");
  }else if(t <= 15 && 80>=h<=95){
    digitalWrite(relay_lampu, LOW);
    digitalWrite(relay_air, HIGH);
    digitalWrite(relay_Kipas, HIGH);
    status_lampu = true;
    Serial.println("lampu nyala");
  }else if(16>=t<=30 && h <= 79){
    digitalWrite(relay_air, LOW);
    digitalWrite(relay_Kipas, HIGH);
    digitalWrite(relay_lampu, HIGH);
    status_air = true;
    Serial.println("Air nyala");
  }else if(16>=t<=30 && h >= 96){
    digitalWrite(relay_Kipas, LOW);
    digitalWrite(relay_lampu, LOW);
    digitalWrite(relay_air, HIGH);
    status_lampu = true;
    status_Kipas = true;
    Serial.println("lampu dan Kipas nyala");
  }else if(t >= 31 && 80>=h<=95){
    digitalWrite(relay_Kipas, LOW);
    digitalWrite(relay_lampu, HIGH);
    digitalWrite(relay_air, HIGH);
    status_Kipas = true;
    Serial.println("Kipas nyala");
  }
}
  
}
