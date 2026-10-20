#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Adafruit_NeoPixel.h>

// --- CẤU HÌNH CHÂN ---
#define LED_PIN 13
#define NUM_LEDS 1
#define BTN_PIN 4     // Nút bấm giả lập giọng nói
#define LDR_PIN 34    // Chân Analog đọc quang trở (giả lập cử chỉ)

// --- CẤU HÌNH MẠNG & MQTT ---
const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* mqtt_server = "broker.hivemq.com";

WiFiClient espClient;
PubSubClient client(espClient);
Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

// --- BIẾN TOÀN CỤC (Chỉ khai báo 1 lần duy nhất ở đây) ---
int currentBrightness = 255;
uint32_t currentColor = strip.Color(255, 255, 255);
bool ledState = false;

int lastLDRValue = 0; 
unsigned long lastTimeReadLDR = 0;

// Khai báo nguyên mẫu hàm
void setup_wifi();
void callback(char* topic, byte* payload, unsigned int length);
void updateLED();

void setup() {
  Serial.begin(115200);
  delay(1000); // Đợi Serial ổn định
  
  strip.begin();
  strip.show();
  
  pinMode(BTN_PIN, INPUT_PULLUP);
  pinMode(LDR_PIN, INPUT);
  lastLDRValue = analogRead(LDR_PIN);

  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void setup_wifi() {
  delay(10);
  Serial.println("\nĐang kết nối WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi đã kết nối.");
}

void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  Serial.println("Nhận lệnh MQTT: " + message);
  
  if (message == "ON") { ledState = true; }
  else if (message == "OFF") { ledState = false; }
  else if (message == "UP") { currentBrightness = min(255, currentBrightness + 50); ledState = true; }
  else if (message == "DOWN") { currentBrightness = max(0, currentBrightness - 50); }
  else if (message.startsWith("COLOR_")) {
    String hex = message.substring(6);
    long number = strtol(&hex[0], NULL, 16);
    currentColor = strip.Color((number >> 16) & 0xFF, (number >> 8) & 0xFF, number & 0xFF);
  }
  updateLED();
}

void updateLED() {
  if (ledState) {
    uint8_t r = ((currentColor >> 16) & 0xFF) * (currentBrightness / 255.0);
    uint8_t g = ((currentColor >> 8) & 0xFF) * (currentBrightness / 255.0);
    uint8_t b = (currentColor & 0xFF) * (currentBrightness / 255.0);
    strip.setPixelColor(0, strip.Color(r, g, b));
  } else {
    strip.setPixelColor(0, strip.Color(0, 0, 0));
  }
  strip.show();
}

void loop() {
  if (!client.connected()) {
    if (client.connect("ESP32_Group15")) {
      client.subscribe("huit/nhom15/light_ctrl");
    }
  }
  client.loop();

  // --- GIẢ LẬP CỬ CHỈ BẰNG QUANG TRỞ (LDR) ---
  if (millis() - lastTimeReadLDR > 200) {
    int currentLDRValue = analogRead(LDR_PIN);
    
    //Serial.print("Giá trị LUX hiện tại: ");
    //Serial.println(currentLDRValue);

    int diff = currentLDRValue - lastLDRValue;
    
    // Ngưỡng nhạy 300
    if (abs(diff) > 300) { 
      if (diff > 0) {
        Serial.println("=> Phát hiện cử chỉ: Vuốt LÊN (Bật đèn)");
        ledState = true;
      } else {
        Serial.println("=> Phát hiện cử chỉ: Vuốt XUỐNG (Tắt đèn)");
        ledState = false;
      }
      updateLED();
    }
    lastLDRValue = currentLDRValue;
    lastTimeReadLDR = millis();
  }

  // --- GIỌNG NÓI (Nút bấm) ---
  if (digitalRead(BTN_PIN) == LOW) {
    delay(50);
    ledState = !ledState;
    updateLED();
    Serial.println("Nút bấm: Đảo trạng thái đèn");
    while(digitalRead(BTN_PIN) == LOW); 
  }

  // --- GIỌNG NÓI (Serial Monitor) ---
  if (Serial.available() > 0) {
    String voiceCmd = Serial.readStringUntil('\n');
    voiceCmd.trim();
    if (voiceCmd == "BAT_DEN") { ledState = true; updateLED(); }
    else if (voiceCmd == "TAT_DEN") { ledState = false; updateLED(); }
  }
}