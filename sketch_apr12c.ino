#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

// Replace with your network credentials
const char* ssid = "For_Experiments";
const char* password = "Tester123456";

ESP8266WebServer server(80);

// Replace with your BME280 sensor address
#define SEALEVELPRESSURE_HPA (1013.25)
Adafruit_BME280 bme; // I2C

// Pin for controlling the heater via relay
const int heaterPin = 17; // Example pin, replace with actual pin number

// Variables to store target temperature and humidity
float targetTemperature = 0.0;
float targetHumidity = 0.0;

void handleRoot() {
  float temperature = bme.readTemperature();
  float humidity = bme.readHumidity();

  String page = "<!DOCTYPE html>\n";
  page += "<html>\n";
  page += "<head><title>Plastic Drying Control</title></head>\n";
  page += "<body>\n";
  page += "<h1>Plastic Drying Control</h1>\n";
  page += "<p>Current Temperature: " + String(temperature) + " °C</p>\n";
  page += "<p>Current Humidity: " + String(humidity) + " %</p>\n";
  page += "<form action='/setParameters' method='post'>\n";
  page += "<p>Target Temperature: <input type='number' name='temperature' value='" + String(targetTemperature) + "'></p>\n";
  page += "<p>Target Humidity: <input type='range' name='humidity' min='0' max='100' value='" + String(targetHumidity) + "'></p>\n";
  page += "<input type='submit' value='Set Parameters'>\n";
  page += "</form>\n";
  page += "</body>\n";
  page += "</html>\n";

  server.send(200, "text/html", page);
}

void handleSetParameters() {
  if (server.hasArg("temperature") && server.hasArg("humidity")) {
    targetTemperature = server.arg("temperature").toFloat();
    targetHumidity = server.arg("humidity").toFloat();
    server.send(200, "text/plain", "Parameters Set");
  } else {
    server.send(400, "text/plain", "Invalid Parameters");
  }
}

void setup() {
  pinMode(heaterPin, OUTPUT);
  digitalWrite(heaterPin, LOW); // Ensure heater is initially off

  Serial.begin(115200);

  // Initialize BME280 sensor
  if (!bme.begin(0x76)) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
    
  }

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.print("Connecting to ");
  Serial.print(ssid);
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(heaterPin, LOW);
    delay(500);
    digitalWrite(heaterPin, HIGH);
    Serial.print(".");
  }
  Serial.println("");
  
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // Routes
  server.on("/", HTTP_GET, handleRoot);
  server.on("/setParameters", HTTP_POST, handleSetParameters);

  server.begin();
  Serial.println("HTTP server started");
}

void loop() {
  server.handleClient();

  float temperature = bme.readTemperature();
  float humidity = bme.readHumidity();

  // Check if current temperature and humidity are below target
  if (temperature < targetTemperature && humidity < targetHumidity) {
    digitalWrite(heaterPin, HIGH); // Turn on heater
  } else {
    digitalWrite(heaterPin, LOW); // Turn off heater
  }

  delay(1000); // Update every second
}
