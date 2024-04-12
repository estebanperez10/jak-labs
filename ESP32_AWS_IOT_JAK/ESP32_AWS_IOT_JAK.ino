
//Añadir librerías
#include "SPIFFS.h"
#include <WiFiClientSecure.h>
#include <Wire.h>
#include <PubSubClient.h>
#include <DHT.h>   
#include <ESP32Time.h>
#include "time.h"             

//Credenciales de red Wifi
//const char* ssid = "VANGUARDV";
//const char* password = "123456789"; 

const char* ssid = "HOH1";
const char* password = "81100628"; 

const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = -18000;
const int   daylightOffset_sec = 0;

//Servidor MQTT
const char* mqtt_server = "a1nb3tsr8z304g-ats.iot.us-east-2.amazonaws.com"; ///////////////////////////////////////////////
const int mqtt_port = 8883;

String Read_rootca;
String Read_cert;
String Read_privatekey;
//********************************
#define BUFFER_LEN  256
long lastMsg = 0;
char msg[BUFFER_LEN];
int value = 0;
long int TIME;
int DATASENSOR;
byte mac[6];
char mac_Id[18];
int count = 1;
//********************************

//Configuración de cliente MQTT
WiFiClientSecure espClient;
PubSubClient client(espClient);

ESP32Time rtc;

//Configuración de DHTXX
#define DHTPIN 19 //Pin
DHT dht(DHTPIN, DHT11); //Modelo

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando.. ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi conectado");
  Serial.println("Direccion IP: ");
  Serial.println(WiFi.localIP());

  //init and get the time
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  printLocalTime();
}
///////////////////////////////////////////////////////////////////////ok 
//Callback
void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensaje recibido [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}

//Conectar a broker MQTT
void reconnect() {
  
  // Loop para reconección
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    
    // Creando un ID como ramdon
    String clientId = "ESP32-";
    clientId += String(random(0xffff), HEX);
    
    // Intentando conectarse
    if (client.connect(clientId.c_str())) {
      Serial.println("conectada");
      
    // Conectado, publicando un payload...
      client.publish("ei_out", "hello world");
    
    // ... y suscribiendo
      client.subscribe("ei_in");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" Esperando 5 segundos");
      
      // Tiempo muerto de 5 segundos
      delay(5000);
    }
  }
}

void setup() {
  
  Serial.begin(115200);
  dht.begin();
  Serial.setDebugOutput(true);
  
  // Inicializa con el PIN led2.
  pinMode(2, OUTPUT);
  setup_wifi();
  delay(1000);
  
  //****************
  if (!SPIFFS.begin(true)) {
    Serial.println("Se ha producido un error al montar SPIFFS");
    return;
  }
  //**********************
  //Root CA leer archivo.
  File file2 = SPIFFS.open("/AmazonRootCA1.pem", "r"); ///////////////////////////////////////////
  if (!file2) {
    Serial.println("No se pudo abrir el archivo para leerlo");
    return;
  }
  Serial.println("Root CA File Content:");
  while (file2.available()) {
    Read_rootca = file2.readString();
    Serial.println(Read_rootca);
  }
  //*****************************
  // Cert leer archivo
  File file4 = SPIFFS.open("/f42e-certificate.pem.crt", "r"); //////////////////////////////////////////
  if (!file4) {
    Serial.println("No se pudo abrir el archivo para leerlo");
    return;
  }
  Serial.println("Cert File Content:");
  while (file4.available()) {
    Read_cert = file4.readString();
    Serial.println(Read_cert);
  }
  //***************************************
  //Privatekey leer archivo
  File file6 = SPIFFS.open("/f42e-private.pem.key", "r"); ///////////////////////////////////////////////
  if (!file6) {
    Serial.println("No se pudo abrir el archivo para leerlo");
    return;
  }
  Serial.println("privateKey contenido:");
  while (file6.available()) {
    Read_privatekey = file6.readString();
    Serial.println(Read_privatekey);
  }
  //=====================================================

  char* pRead_rootca;
  pRead_rootca = (char *)malloc(sizeof(char) * (Read_rootca.length() + 1));
  strcpy(pRead_rootca, Read_rootca.c_str());

  char* pRead_cert;
  pRead_cert = (char *)malloc(sizeof(char) * (Read_cert.length() + 1));
  strcpy(pRead_cert, Read_cert.c_str());

  char* pRead_privatekey;
  pRead_privatekey = (char *)malloc(sizeof(char) * (Read_privatekey.length() + 1));
  strcpy(pRead_privatekey, Read_privatekey.c_str());

  Serial.println("================================================================================================");
  Serial.println("Certificados que pasan adjuntan al espClient");
  Serial.println();
  Serial.println("Root CA:");
  Serial.write(pRead_rootca);
  Serial.println("================================================================================================");
  Serial.println();
  Serial.println("Cert:");
  Serial.write(pRead_cert);
  Serial.println("================================================================================================");
  Serial.println();
  Serial.println("privateKey:");
  Serial.write(pRead_privatekey);
  Serial.println("================================================================================================");

  espClient.setCACert(pRead_rootca);
  espClient.setCertificate(pRead_cert);
  espClient.setPrivateKey(pRead_privatekey);

  client.setServer(mqtt_server, mqtt_port);
  client.setCallback(callback);

  //******************************************
  WiFi.macAddress(mac);
  snprintf(mac_Id, sizeof(mac_Id), "%02x%02x%02x%02x%02x%02x",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.println(mac_Id);
  //****************************************
  delay(2000);
 
}


void loop() {

  // ******************** SENSOR DHTxx *********************************
//  float h = dht.readHumidity();         // Lectura de Temperatura
//  float t = dht.readTemperature();      // Lectura de humedad
//  if (isnan(h) || isnan(t))
//  {
//    Serial.println("¡Error al leer del sensor DHT!");
//    return;
//  }

  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  printLocalTime();
  long now = millis();
  if (now - lastMsg > 5000) {
    lastMsg = now;
    //=============================================================================================
//    TIME = random(1712189912);
    DATASENSOR = random(3000,4000);
    String macIdStr = mac_Id;
//    snprintf (msg, BUFFER_LEN, "{\"messageID\" : \"sensor-data\", \"timestamp\" : 51746634, \"mac\" : \"25278725d\", \"sensorData\" : \"5445\", \"location\" : \"1\"}");
    snprintf (msg, BUFFER_LEN, "{\"messageID\" : \"sensor-data\", \"timestamp\" : %d, \"mac\" : \"%s\", \"sensorData\" : \"%.4d\", \"location\" : \"1\"}", rtc.getEpoch(), macIdStr.c_str(), DATASENSOR);
//    snprintf (msg, BUFFER_LEN, "{\"messageID\" : \"sensor-data\", \"timestamp\" : %.10d, \"mac\" : \"%s\", \"sensorData\" : \"%.4d\", \"location\" : \"1\"}", TIME, macIdStr.c_str(), DATASENSOR);
//    snprintf (msg, BUFFER_LEN, "{\"mac_Id\" : \"%s\", \"Temperatura\" : %s, \"Humedad\" : %s}", macIdStr.c_str(), Temprature.c_str(), Humidity.c_str());
    Serial.print("Publicando mensaje: ");
    Serial.print(count);
    Serial.println(msg);
    client.publish("jak/sensor", msg); //topic
    count = count + 1;
    //================================================================================================
  }
  digitalWrite(2, HIGH);   
  delay(1000);                      
  digitalWrite(2, LOW);   
  delay(1000);                       
}

void printLocalTime()
{
  struct tm timeinfo;
  if(!getLocalTime(&timeinfo)){
    Serial.println("Failed to obtain time");
    return;
  }
//  Serial.println(&timeinfo, "%d %Y %H:%M:%S");
}
