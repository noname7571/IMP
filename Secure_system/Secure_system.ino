#include "HTTPClient.h"
#include <WiFi.h>


#define PIR_SENSOR  26 // PIR motion sensor pin

// WiFi credentials
const char* ssid = "IoT_2G";
const char* password = "23101999";

void setup() {
  Serial.begin(115200); // Serial communication initialization

  pinMode(PIR_SENSOR, INPUT); // Configure PIR motion sensor pin as input

  // Connect to WiFi
  Serial.println();
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting...");
  }

  // Display connected IP address
  Serial.print("Connected to IP Address: http://");
  Serial.println(WiFi.localIP());
  Serial.println();
}

void loop() {
  int sensorValue = digitalRead(PIR_SENSOR); // Read the PIR sensor value

  // Check for motion detection
  if (sensorValue == 1) {
    Serial.println("Motion Detected!");

    // Create WiFiClient and HTTPClient instances 
    WiFiClient client;
    HTTPClient http;
    http.begin(client, "http://192.168.0.146/capture"); // Specify the target URL for the HTTP request
    http.GET(); // Send a GET request to the specified URL
  }
}
