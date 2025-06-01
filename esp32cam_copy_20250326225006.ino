#include "esp_camera.h"
#include "Arduino.h"
#include "FS.h"
#include "SD_MMC.h"
#include <NTPClient.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <ESPAsyncWebServer.h>

#define EEPROM_SIZE 4
// Pin definition for CAMERA_MODEL_AI_THINKER
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

// WiFi 和 NTP 配置
const char* ssid = "346"; // 替换为你的WiFi名称
const char* password = "34666666"; // 替换为你的WiFi密码
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "ntp.aliyun.com"); // 使用阿里云的 NTP 服务器

// Web 服务器配置
AsyncWebServer server(80);

int pictureNumber = 0;
unsigned long lastCaptureTime = 0; // 上一次拍照的时间
const unsigned long interval = 30000; // 拍照间隔，单位为毫秒，10000ms = 10秒
String lastPictureName = ""; // 存储最后一张照片的名称

bool sdCardInitialized = false; // 用于标记 SD 卡是否初始化成功

bool takePictureAndSave(); // 函数声明

void setup() {
  Serial.begin(115200);

  // 逐步启动各个模块
  Serial.println("Starting WiFi...");
  startWiFi(); // 启动 WiFi 模块
  delay(5000); // 等待 5 秒，确保 WiFi 连接稳定
  Serial.println("Starting Web Server...");
  startWebServer(); // 启动 Web 服务器
  Serial.println("Starting NTP...");
  startNTP(); // 启动 NTP 模块
  delay(5000); // 等待 5 秒，确保 NTP 时间同步

  Serial.println("Initializing SD Card...");
  if (!initSDCard()) { // 初始化 SD 卡
    Serial.println("Failed to initialize SD Card after multiple attempts.");
    while (1) {
      delay(1000); // 如果 SD 卡初始化失败，进入死循环
    }
  }
  delay(5000); // 等待 5 秒，确保 SD 卡初始化完成

  Serial.println("Initializing Camera...");
  initCamera(); // 初始化摄像头
  delay(5000); // 等待 5 秒，确保摄像头初始化完成

  Serial.println("All modules started successfully!");
}

void loop() {
  if (millis() - lastCaptureTime >= interval) { // 检查是否达到拍照间隔
    takePictureAndSave();
    lastCaptureTime = millis(); // 更新拍照时间
  }
}

void startWiFi() {
  WiFi.begin(ssid, password); // 连接WiFi
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi");
}

void startNTP() {
  timeClient.begin(); // 初始化NTP客户端
  timeClient.update(); // 获取当前时间
  Serial.println("NTP Time Synced");
}

bool initSDCard() {
  int attempts = 0;
  while (attempts < 5) { // 尝试 5 次
    if (SD_MMC.begin("/sdcard", true)) {
      uint8_t cardType = SD_MMC.cardType();
      if (cardType == CARD_NONE) {
        Serial.println("No SD Card attached");
      } else {
        Serial.println("SD Card initialized successfully");
        sdCardInitialized = true;
        return true;
      }
    } else {
      Serial.println("SD Card Mount Failed, retrying...");
    }
    delay(1000);
    attempts++;
  }
  return false; // 如果多次尝试失败，返回 false
}

void initCamera() {
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
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }
  Serial.println("Camera initialized successfully");
}

bool takePictureAndSave() {
  if (!sdCardInitialized) {
    Serial.println("SD Card is not initialized, cannot save picture.");
    return false;
  }

  camera_fb_t * fb = NULL;
  fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    return false;
  }

  // 获取当前时间并格式化为年月日时分秒（北京时间，UTC+8）
  timeClient.update();
  time_t rawTime = timeClient.getEpochTime() + 8 * 3600; // 转换为北京时间
  struct tm *timeinfo;
  timeinfo = gmtime(&rawTime);
  char timestamp[20];
  strftime(timestamp, sizeof(timestamp), "%Y%m%d%H%M%S", timeinfo);

  String path = "/sdcard/picture_" + String(timestamp) + ".jpg";
  fs::FS &fs = SD_MMC;
  Serial.printf("Picture file name: %s\n", path.c_str());

  // 检查根目录是否存在
  if (!fs.exists("/")) {
    Serial.println("SD Card root directory does not exist.");
    esp_camera_fb_return(fb);
    return false;
  }

  // 尝试创建文件夹（如果需要）
  if (!fs.exists("/pictures")) {
    if (!fs.mkdir("/pictures")) {
      Serial.println("Failed to create /pictures directory.");
      esp_camera_fb_return(fb);
      return false;
    }
  }

  // 使用绝对路径保存文件
  path = "/pictures/picture_" + String(timestamp) + ".jpg";
  Serial.printf("Final picture file path: %s\n", path.c_str());

  File file = fs.open(path.c_str(), FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file in writing mode");
    esp_camera_fb_return(fb);
    return false;
  }

  if (file.write(fb->buf, fb->len) != fb->len) {
    Serial.println("Failed to write data to file");
    file.close();
    esp_camera_fb_return(fb);
    return false;
  } else {
    Serial.printf("Saved file to path: %s\n", path.c_str());
    lastPictureName = path; // 更新最后一张照片的名称
  }
  file.close();
  esp_camera_fb_return(fb);
  pictureNumber++;
  return true;
} 

void startWebServer() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    String html = R"rawliteral(
      <!DOCTYPE html>
      <html lang="en">
      <head>
        <meta charset="UTF-8">
        <meta name="viewport" content="width=device-width, initial-scale=1.0">
        <title>酥艾的ESP给你拍的照</title>
        <style>
          body {
            font-family: Arial, sans-serif;
            background-color: #f4f4f4;
            margin: 0;
            padding: 0;
          }
          .container {
            max-width: 800px;
            margin: 0 auto;
            padding: 20px;
            background-color: #fff;
            box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
          }
          h1 {
            color: #333;
          }
          p {
            color: #666;
            font-size: 18px;
          }
          img {
            max-width: 100%;
            height: auto;
            border: 2px solid #ddd;
            border-radius: 5px;
          }
          .button {
            display: inline-block;
            padding: 10px 20px;
            margin: 10px 0;
            background-color: #007BFF;
            color: #fff;
            text-decoration: none;
            border-radius: 5px;
            font-size: 16px;
          }
          .button:hover {
            background-color: #0056b3;
          }
        </style>
      </head>
      <body>
        <div class="container">
          <h1>照片在下面</h1>
          <p>拍摄的最后一张照片名称: <span id="pictureName">)rawliteral" + lastPictureName + R"rawliteral(</span></p>
          <img id="lastPicture" src="/lastPicture" alt="Last Picture" />
          <a href="/takePicture" class="button">Take Picture</a>
        </div>
      </body>
      </html>
    )rawliteral";

    request->send(200, "text/html", html);
  });

  // 添加一个用于显示最后一张照片的路由 - 修复版本
  server.on("/lastPicture", HTTP_GET, [](AsyncWebServerRequest *request) {
    if (lastPictureName.length() > 0) {
      // 使用文件路径发送图片
      if (SD_MMC.exists(lastPictureName)) {
        request->send(SD_MMC, lastPictureName, "image/jpeg");
        Serial.printf("[INFO] Sent photo: %s\n", lastPictureName.c_str());
      } else {
        Serial.printf("[ERROR] File not found: %s\n", lastPictureName.c_str());
        request->send(404, "text/plain", "File not found");
      }
    } else {
      Serial.println("[WARN] No pictures available");
      request->send(404, "text/plain", "No picture available");
    }
  });

  // 添加一个用于拍照的路由
  server.on("/takePicture", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("[ACTION] Taking picture...");
    
    if (takePictureAndSave()) {
      Serial.printf("[SUCCESS] Saved: %s\n", lastPictureName.c_str());
      request->send(200, "text/plain", "Picture 3taken");
    } else {
      Serial.println("[ERROR] Capture failed");
      request->send(500, "text/plain", "Capture failed");
    }
  });
  
  server.begin();
  Serial.println("Web server started");
}