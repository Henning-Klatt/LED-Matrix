#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <SPI.h>
#include <bitBangedSPI.h>
#include <MAX7219_Dot_Matrix.h>
const byte chips = 4;
const char* mqtt_server = "10.11.12.1";
bool lauftext = false;

ESP8266WiFiMulti WiFiMulti;
WiFiClient espClient;
PubSubClient client(espClient);

// 12 chips (display modules), hardware SPI with load on D10
MAX7219_Dot_Matrix display (chips, 2);  // Chips / LOAD 

char message [] = "Waiting for data to show.";

void connect(){
  //WiFi.mode(WIFI_OFF);
  WiFiMulti.addAP("smartHome", "Tm_77?w8bhP");

  Serial.println();
  Serial.println();
  Serial.print("Wait for WiFi... ");

  while(WiFiMulti.run() != WL_CONNECTED) {
    Serial.print(".");
    delay(500);
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  
}

void callback(char* topic, byte* payload, unsigned int length) {
  payload[length] = '\0';
  char* input = (char*)payload;
  String strTopic = String((char*)topic);
  Serial.print("MQTT [");
  Serial.print(strTopic);
  Serial.print("] ");
  Serial.println(input);
  if(strTopic == "matrix/brightness"){
    int value = atoi(input);
    if(value >= 0 && value <= 15){
      Serial.print("Helligkeit: ");
      Serial.println(value);
      display.setIntensity(value);
    }
  }
  if(strTopic == "matrix/sendSmooth"){
    Serial.println("Lauftext: ");
    Serial.println(input);
    String NewMessage = input;
    NewMessage.toCharArray(message, 5000);
    //updateDisplay();
    lauftext = true;
  }
  if(strTopic == "matrix/sendString"){
    Serial.println("Text: ");
    Serial.println(input);
    char* message = input;
    lauftext = false;
    display.sendString(message);
  }
}
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    display.sendString("MQTT");
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    if (client.connect(clientId.c_str()))
    {
      Serial.println("connected");
      client.subscribe("matrix/#");
      lauftext = true;
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      lauftext = false;
      delay(5000);
    }
  }
}
 
void setup () {
  Serial.begin(9600);
  display.begin ();
  display.setIntensity(15);
  display.sendString("WiFi");
  connect();
  display.sendString("MQTT");
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  String ip = WiFi.localIP().toString();
  ip.toCharArray(message, 50);
  lauftext = true;
}

unsigned long lastMoved = 0;
unsigned long MOVE_INTERVAL = 40;  // mS
int  messageOffset;

void updateDisplay (){
  display.sendSmooth(message, messageOffset);
  if (messageOffset++ >= (int) (strlen (message) * 8))
  messageOffset = - chips * 8;
}

void loop () { 
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  if(lauftext){
    if (millis () - lastMoved >= MOVE_INTERVAL) {
      updateDisplay ();
      lastMoved = millis ();
    }
  }
}
