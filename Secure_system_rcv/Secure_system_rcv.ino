#include <WiFi.h>
#include "esp_camera.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include "driver/rtc_io.h"
#include "esp_timer.h"
#include "img_converters.h"
#include "ESPAsyncWebServer.h"
#include "Arduino.h"
#include "SPIFFS.h"
#include "Arduino.h"
#include "FS.h"

// WiFi credentials
const char* ssid = "IoT_2G";
const char* password = "23101999";

// AsyncWebServer instance
AsyncWebServer server(80);

// Flag to trigger a new photo capture
bool takeNewPhoto = false;

// GPIO Pin Definitions for AI Thinker ESP32-CAM
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22
#define LED_BUILTIN        4 // Flash pin


// HTML page for the web server
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body {
      text-align: center;
      font-family: 'Arial', sans-serif;
      background-color: #f4f4f4;
      margin: 0;
      padding: 0;
    }

    #container {
      margin-top: 20px;
      padding: 20px;
      background-color: #ffffff;
      box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
      border-radius: 10px;
    }

    h2 {
      color: #333333;
    }

    p {
      margin: 10px 0;
      color: #666666;
    }

    button {
      padding: 10px;
      margin: 5px;
      background-color: #4CAF50;
      color: #ffffff;
      border: none;
      border-radius: 5px;
      cursor: pointer;
      font-size: 14px;
    }

    button:hover {
      background-color: #45a049;
    }

    #photo {
      width: 70%;
      margin-top: 20px;
      border: 1px solid #dddddd;
      border-radius: 5px;
    }

    .vert {
      margin-bottom: 10%;
    }

    .hori {
      margin-bottom: 0%;
    }

    footer {
      margin-top: 20px;
      padding: 10px;
      background-color: #333333;
      color: #ffffff;
      border-radius: 5px;
    }
  </style>
</head>
<body>
  <div id="container">
    <h1>Š - ESP32: Zabezpečovací systém s detekcí pohybu</h1>
    <p>It takes up to 4 seconds to capture a photo. Please refresh the page after that.</p>
    <p>It captures a photo automatically when it detects motion using a PIR motion sensor.</p>
    <p>You can also capture a photo using a button, even if it did not detect any motion.</p>
    <p>If the captured photo is inverted or not set properly, you can rotate it.</p>
    <p>
      <button onclick="rotatePhoto();">ROTATE</button>
      <button onclick="capturePhoto();">CAPTURE PHOTO</button>
      <button onclick="location.reload();">REFRESH PAGE</button>
    </p>
  </div>
  <div>
    <img src="saved-photo" id="photo" alt="Last Captured Photo">
  </div>
  <footer>
    <p>Matej Tomko, xtomko06</p>
  </footer>
  <script>
    var deg = 0;
    function capturePhoto() {
      var xhr = new XMLHttpRequest();
      xhr.open('GET', "/capture", true);
      xhr.send();
    }
    function rotatePhoto() {
      var img = document.getElementById("photo");
      deg += 90;
      if (isOdd(deg / 90)) {
        document.getElementById("container").className = "vert";
      } else {
        document.getElementById("container").className = "hori";
      }
      img.style.transform = "rotate(" + deg + "deg)";
    }
    function isOdd(n) {
      return Math.abs(n % 2) == 1;
    }
    function handleRefresh() {
    location.reload();
    }
    var socket = new WebSocket('ws://' + window.location.hostname + ':81/');
    socket.onmessage = function (event) {
      if (event.data === 'Refresh') {
        handleRefresh();
      }
    };
  </script>
</body>
</html>)rawliteral";


// Function to check if a new photo exists
bool checkPhoto( fs::FS &fs ) {
  File f_pic = fs.open( "/photo.jpg" );
  unsigned int pic_sz = f_pic.size();
  return ( pic_sz > 100 );
}


// Function to capture and save a new photo
void capturePhoto( void ) {
  camera_fb_t * fb = NULL;
  bool ok = false;

  do {
    Serial.println("Taking a photo...");

    // Flash the light and take a new picture
    digitalWrite(LED_BUILTIN, HIGH);
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      return;
    }
    digitalWrite(LED_BUILTIN, LOW);

    // Save the photo to SPIFFS
    Serial.printf("Picture file name: %s\n", "/photo.jpg");
    File file = SPIFFS.open("/photo.jpg", FILE_WRITE);

    if (!file) {
      Serial.println("Failed to open file in writing mode");
    }
    else {
      file.write(fb->buf, fb->len); // payload (image), payload length
      Serial.print("The picture has been saved in ");
      Serial.print("/photo.jpg");
      Serial.print(" - Size: ");
      Serial.print(file.size());
      Serial.println(" bytes");
    }

    file.close();
    esp_camera_fb_return(fb);

    // Check if the photo was saved successfully
    ok = checkPhoto(SPIFFS);
  } while ( !ok ); // Do all that after it was saved successfully
}

void setup() {
  Serial.begin(115200); // Serial communication initialization

  pinMode(LED_BUILTIN, OUTPUT); // Configure built in LED pin as output

  // Connect to WiFi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }

  // Mount SPIFFS file system
  if (!SPIFFS.begin(true)) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    ESP.restart();
  } else {
    delay(500);
    Serial.println("SPIFFS mounted successfully");
  }

  // Display server IP address
  Serial.print("IP Address: http://");
  Serial.println(WiFi.localIP());


  // Configure camera settings
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // Disable the brownout detector

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;

  if (psramFound()) {
    config.frame_size = FRAMESIZE_UXGA;
    config.jpeg_quality = 10;
    config.fb_count = 2;
  } else {
    config.frame_size = FRAMESIZE_SVGA;
    config.jpeg_quality = 12;
    config.fb_count = 1;
  }

  // Initialize the camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    ESP.restart();
  }


  // Configure the routes of the web server
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/html", index_html);
  });

  server.on("/capture", HTTP_GET, [](AsyncWebServerRequest * request) {
    takeNewPhoto = true;
    request->send_P(200, "text/plain", "Taking Photo");
    delay(3000);
    request->send_P(200, "text/html", index_html);
  });

  server.on("/saved-photo", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(SPIFFS, "/photo.jpg", "image/jpg", false);
  });

  server.begin(); // Start the web server
}

void loop() {
  // Check if a new photo needs to be captured
  if (takeNewPhoto) {
    capturePhoto();
    takeNewPhoto = false;
  }
}
