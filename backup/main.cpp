#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>

// ========== НАСТРОЙКИ ========== //
// Джойстик 1
#define JOY1_X_PIN 32  // X-ось джойстика 1
#define JOY1_Y_PIN 33  // Y-ось джойстика 1
// Джойстик 2
#define JOY2_X_PIN 34  // X-ось джойстика 2
#define JOY2_Y_PIN 35  // Y-ось джойстика 2
// Кнопки джойстиков
#define JOY1_BTN_PIN 12  // Кнопка джойстика 1
#define JOY2_BTN_PIN 14  // Кнопка джойстика 2

const uint32_t SEND_DELAY = 20; // Задержка между отправками (мс)
const int DEADZONE = 50;  // Мертвая зона джойстика

typedef struct {
  int16_t joy1_x;
  int16_t joy1_y;
  int16_t joy2_x;
  int16_t joy2_y;
  bool joy1_btn;
  bool joy2_btn;
  uint8_t batteryLevel;
} ControllerData;

ControllerData txData;
// Замените на MAC-адрес вашего приёмника
uint8_t receiverMac[] = {0x94, 0x3C, 0xC6, 0x33, 0x71, 0x68};

// Калибровка джойстика
int16_t calibrateJoystick(int rawValue) {
  int16_t mapped = map(rawValue, 0, 4095, -512, 512);
  if (abs(mapped) < DEADZONE) return 0;
  return mapped;
}

void setup() {
  Serial.begin(115200);
  delay(100);
  
  // Настройка пинов
  pinMode(JOY1_BTN_PIN, INPUT_PULLUP);
  pinMode(JOY2_BTN_PIN, INPUT_PULLUP);

  // Инициализация WiFi
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Ошибка инициализации ESP-NOW");
    ESP.restart();
  }

  esp_now_peer_info_t peerInfo;
  memset(&peerInfo, 0, sizeof(peerInfo));
  memcpy(peerInfo.peer_addr, receiverMac, 6);
  peerInfo.channel = 1;
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Ошибка добавления получателя");
    ESP.restart();
  }

  Serial.println("Контроллер готов к работе");
}

void sendData() {
  // Чтение джойстиков
  txData.joy1_x = calibrateJoystick(analogRead(JOY1_X_PIN));
  txData.joy1_y = calibrateJoystick(analogRead(JOY1_Y_PIN));
  txData.joy2_x = calibrateJoystick(analogRead(JOY2_X_PIN));
  txData.joy2_y = calibrateJoystick(analogRead(JOY2_Y_PIN));
  
  // Чтение кнопок (инвертируем из-за PULLUP)
  txData.joy1_btn = !digitalRead(JOY1_BTN_PIN);
  txData.joy2_btn = !digitalRead(JOY2_BTN_PIN);
  
  // Чтение батареи (опционально)
  txData.batteryLevel = map(analogRead(36), 0, 4095, 0, 100);

  // Отправка данных
  esp_err_t result = esp_now_send(receiverMac, (uint8_t *)&txData, sizeof(txData));
  
  // Отладочная информация
  Serial.printf("J1: %d,%d BTN:%d | J2: %d,%d BTN:%d | Status: %s\n",
    txData.joy1_x, txData.joy1_y, txData.joy1_btn,
    txData.joy2_x, txData.joy2_y, txData.joy2_btn,
    result == ESP_OK ? "OK" : "FAIL");
}

void loop() {
  sendData();
  delay(SEND_DELAY);
}