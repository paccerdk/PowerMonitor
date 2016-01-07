#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>

#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <WebSocketsServer.h>   //https://github.com/Links2004/arduinoWebSockets
#include "FS.h" //for SPIFFS filesystem

#define ENABLE_LED true

ESP8266WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);

void handleNotFound();
void handleRoot();
void handleBlink();
String getContentType(String filename);
bool handleFileRead(String path);
void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length);


int blinks = 0;
bool newblink = false;
int timestamp = 0;

uint32_t blinkTime;
uint32_t lastBlinkTime;
int wattConsumption;

int websocketClients = 0;

void setup() {
    // put your setup code here, to run once:
    Serial.begin(115200);
    SPIFFS.begin();
    
    //WiFiManager
    //Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;
    //reset saved settings
    //wifiManager.resetSettings();
    
	//username, password
    wifiManager.autoConnect("ESPConfig", "password");


    //if you get here you have connected to the WiFi
    if (ENABLE_LED) {
      pinMode(1, OUTPUT);
    }
    pinMode(2, INPUT);
    attachInterrupt(digitalPinToInterrupt(2), handleBlink, FALLING);
    
    //server.on("/", handleRoot);
    //server.onNotFound(handleNotFound);
    server.begin();
    webSocket.begin();
    webSocket.onEvent(webSocketEvent);

    //called when the url is not defined here
    //use it to load content from SPIFFS
    server.onNotFound([](){
    if(!handleFileRead(server.uri()))
      server.send(404, "text/plain", "FileNotFound");
    });
}

void loop() {
    // put your main code here, to run repeatedly:
    server.handleClient();
    webSocket.loop();
/*
    if (millis() > timestamp+2000) {
        //webSocket.broadcastTXT(String(random(1000)));
        webSocket.broadcastTXT(String(random(1000)));
        timestamp = millis();
     }
*/
    if (ENABLE_LED) {
      if (millis() > timestamp+10) {
        digitalWrite(1, HIGH);
      }
    }
    if (newblink) {
        webSocket.broadcastTXT(String(wattConsumption));
        Serial.println(blinks);
        newblink = false;
    }
}

void handleBlink() {
  //Serial.print("Blink ");
  lastBlinkTime = blinkTime;
  blinkTime = millis();
  wattConsumption = 360000 / (blinkTime - lastBlinkTime); //10.000 blink/kWh

  newblink = true;
  blinks++;
  //Serial.println(blinks);
  if (ENABLE_LED) {
    digitalWrite(1, LOW);
    timestamp = millis();
  }
  
}

String getContentType(String filename){
  if(server.hasArg("download")) return "application/octet-stream";
  else if(filename.endsWith(".htm")) return "text/html";
  else if(filename.endsWith(".html")) return "text/html";
  else if(filename.endsWith(".css")) return "text/css";
  else if(filename.endsWith(".js")) return "application/javascript";
  else if(filename.endsWith(".png")) return "image/png";
  else if(filename.endsWith(".gif")) return "image/gif";
  else if(filename.endsWith(".jpg")) return "image/jpeg";
  else if(filename.endsWith(".ico")) return "image/x-icon";
  else if(filename.endsWith(".xml")) return "text/xml";
  else if(filename.endsWith(".pdf")) return "application/x-pdf";
  else if(filename.endsWith(".zip")) return "application/x-zip";
  else if(filename.endsWith(".gz")) return "application/x-gzip";
  return "text/plain";
}

bool handleFileRead(String path){
  Serial.println("handleFileRead: " + path);
  if(path.endsWith("/")) path += "index.htm";
  String contentType = getContentType(path);
  Serial.print("Content type: ");
  Serial.println(contentType);
  String pathWithGz = path + ".gz";
  if(SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)){
    if(SPIFFS.exists(pathWithGz))
      path += ".gz";
    File file = SPIFFS.open(path, "r");
    size_t sent = server.streamFile(file, contentType);
    file.close();
    return true;
  }
  return false;
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t * payload, size_t length) {

    switch(type) {
        case WStype_DISCONNECTED:
            Serial.print("Client ");
            Serial.print(num);
            Serial.println(" Disconnected!");
            break;
            
        case WStype_CONNECTED:
            IPAddress ip = webSocket.remoteIP(num);
            Serial.print("Client ");
            Serial.print(num);
            Serial.print(" Connected from ");
            Serial.println(ip);
            break;
    }
    
}
