e#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <FS.h>
#include <ESP8266HTTPClient.h>

const char* ssid = "SSID";
const char* password = "PASSWORD";

AsyncWebServer server(80);
String serverIP;
String deviceName;

os_timer_t myTimer;
volatile bool sendRequestFlag = false;

// Предварительное объявление функций
void onTimer();
void sendHttpRequest();
String readIPFromFile();
String readDeviceNameFromFile();
void writeIPToFile(String ip);
void writeDeviceNameToFile(String name);

String readIPFromFile() {
  if (!SPIFFS.begin()) {
    Serial.println("An error has occurred while mounting SPIFFS");
    return "";
  }
  
  File file = SPIFFS.open("/serverIP.txt", "r");
  if (!file) {
    Serial.println("Failed to open file for reading");
    return "";
  }
  
  String ip = file.readString();
  file.close();
  return ip;
}

void writeIPToFile(String ip) {
  if (!SPIFFS.begin()) {
    Serial.println("An error has occurred while mounting SPIFFS");
    return;
  }
  
  File file = SPIFFS.open("/serverIP.txt", "w");
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  
  file.print(ip);
  file.close();
}


String readDeviceNameFromFile() {
  if (!SPIFFS.begin()) {
    Serial.println("An error has occurred while mounting SPIFFS");
    return "";
  }
  
  File file = SPIFFS.open("/deviceName.txt", "r");
  if (!file) {
    Serial.println("Failed to open file for reading");
    return "";
  }
  
  String name = file.readString();
  file.close();
  return name;
}

void writeDeviceNameToFile(String name) {
  if (!SPIFFS.begin()) {
    Serial.println("An error has occurred while mounting SPIFFS");
    return;
  }
  
  File file = SPIFFS.open("/deviceName.txt", "w");
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  
  file.print(name);
  file.close();
}

void setup() {
  Serial.begin(115200);

  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
    Serial.println(WiFi.macAddress());
  }
  Serial.println("Connected to WiFi");
  Serial.println(WiFi.localIP());


  serverIP = readIPFromFile();
  if (serverIP.length() > 0) {
    Serial.println("Loaded server IP: " + serverIP);
  }
  
  deviceName = readDeviceNameFromFile();
  if (deviceName.length() > 0) {
    Serial.println("Loaded device name: " + deviceName);
  }

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send(200, "text/html", "<form action=\"/setIP\" method=\"POST\"><input type=\"text\" name=\"ip\" placeholder=\"Server IP\"><input type=\"text\" name=\"device\" placeholder=\"Device Name\"><input type=\"submit\" value=\"Set IP and Device Name\"></form>");
  });

  server.on("/setIP", HTTP_POST, [](AsyncWebServerRequest *request){
    if (request->hasParam("ip", true) && request->hasParam("device", true)) {
      serverIP = request->getParam("ip", true)->value();
      deviceName = request->getParam("device", true)->value();
      writeIPToFile(serverIP);
      writeDeviceNameToFile(deviceName);
      request->send(200, "text/html", "IP set to: " + serverIP + "<br>Device name set to: " + deviceName);
    } else {
      request->send(200, "text/html", "No IP or Device Name received");
    }
  });

  server.begin();


  os_timer_setfn(&myTimer, (os_timer_func_t*)onTimer, NULL);
  os_timer_arm(&myTimer, 30000, true);
}

void loop() {
  if (sendRequestFlag) {
    sendRequestFlag = false;
    sendHttpRequest();
  }
}

void onTimer() {
  sendRequestFlag = true;
}

void sendHttpRequest() {
  if (serverIP.length() > 0 && deviceName.length() > 0) {
    WiFiClient client;
    HTTPClient http;
    String url = "http://" + serverIP + "/live?name=" + deviceName;

    http.begin(client, url);
    int httpResponseCode = http.GET();
    
    if (httpResponseCode > 0) {
      Serial.println("HTTP Response code: " + String(httpResponseCode));
    } else {
      Serial.println("Error on sending request: " + http.errorToString(httpResponseCode));
    }
    
    http.end();
  } else {
    Serial.println("Server IP or Device Name is not set");
  }
}
