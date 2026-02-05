#include <Arduino.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <DHT.h>

// WiFi配置
const char* ssid = "Tenda_EEB140";
const char* password = "13335465866";

// DHT配置
#define DHTPIN 4
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Thingspeak配置
const char* thingspeakAPIKey = "FK95DBGSVDCX9SP3"; // ← 重要！
const char* thingspeakServer = "http://api.thingspeak.com/update";

// 上传间隔（Thingspeak免费版限制15秒）
const unsigned long uploadInterval = 20000; // 20秒（留有余量）

unsigned long lastUploadTime = 0;

void setup() {
  Serial.begin(9600);
  
  // 连接WiFi
  connectToWiFi();
  
  // 初始化DHT
  dht.begin();
  
  Serial.println("系统启动完成");
}

void loop() {
  // 检查WiFi连接
  if (WiFi.status() != WL_CONNECTED) {
    reconnectWiFi();
  }
  
  // 定时上传数据（遵守Thingspeak限制）
  if (millis() - lastUploadTime > uploadInterval) {
    float temperature = dht.readTemperature();
    float humidity = dht.readHumidity();
    
    if (!isnan(temperature) && !isnan(humidity)) {
      uploadToThingspeak(temperature, humidity);
      lastUploadTime = millis();
    } else {
      Serial.println("读取传感器失败");
    }
  }
  
  delay(1000);
}

void connectToWiFi() {
  Serial.print("连接到WiFi: ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 30) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi连接成功!");
    Serial.print("IP地址: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nWiFi连接失败!");
  }
}

void reconnectWiFi() {
  Serial.println("WiFi断开，尝试重连...");
  WiFi.disconnect();
  WiFi.reconnect();
  
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) {
    delay(500);
    Serial.print(".");
    attempts++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi重连成功!");
  }
}

void uploadToThingspeak(float temperature, float humidity) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    
    // 构建URL（GET方式）
    String url = String(thingspeakServer);
    url += "?api_key=" + String(thingspeakAPIKey);
    url += "&field1=" + String(temperature, 1);  // 温度
    url += "&field2=" + String(humidity, 1);     // 湿度
    
    Serial.print("上传数据到Thingspeak: ");
    Serial.println(url);
    
    // 发送HTTP GET请求
    http.begin(url);
    int httpCode = http.GET();
    
    if (httpCode > 0) {
      Serial.printf("HTTP响应代码: %d\n", httpCode);
      
      if (httpCode == HTTP_CODE_OK) {
        String payload = http.getString();
        Serial.println("上传成功");
        Serial.println("响应: " + payload);
        
        // 响应"1"表示成功，其他数字表示记录ID
      }
    } else {
      Serial.printf("HTTP请求失败: %s\n", http.errorToString(httpCode).c_str());
    }
    
    http.end();
  } else {
    Serial.println("WiFi未连接，无法上传数据");
  }
}