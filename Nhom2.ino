#include <Wire.h>
#include <LiquidCrystal_PCF8574.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <BH1750.h>

// Địa chỉ I2C của LCD
LiquidCrystal_PCF8574 lcd(0x27);

// DS18B20
#define ONE_WIRE_BUS 5

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

BH1750 lightMeter;

// Nút bấm
#define BUTTON1_PIN 16 
#define BUTTON2_PIN 17
#define BUTTON3_PIN 23 
#define BUTTON4_PIN 19 

// Relay
#define RELAY_LIGHT_PIN 15
#define RELAY_FAN_PIN 2

// Ngưỡng
float tempThreshold = 25.0;
float luxThreshold = 100.0;

// Trạng thái của menu
enum MenuState {
  STATE_DISPLAY,
  STATE_SET_TEMP_THRESHOLD,
  STATE_SET_LUX_THRESHOLD,
  STATE_PAUSE
};
MenuState currentState = STATE_SET_TEMP_THRESHOLD;
bool systemPaused = true;

void setup() {
  Serial.begin(115200);
  
  // Khởi tạo LCD
  lcd.begin(16, 2);
  lcd.setBacklight(255);
  lcd.setCursor(0, 0);
  lcd.print("Starting...");

  // Khởi tạo cảm biến nhiệt độ DS18B20
  sensors.begin();

  // Khởi tạo cảm biến ánh sáng BH1750
  Wire.begin();
  lightMeter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE);

  // Khởi tạo các nút bấm
  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);
  pinMode(BUTTON3_PIN, INPUT_PULLUP);
  pinMode(BUTTON4_PIN, INPUT_PULLUP);

  // Khởi tạo các relay
  pinMode(RELAY_LIGHT_PIN, OUTPUT);
  pinMode(RELAY_FAN_PIN, OUTPUT);

  // Tắt các relay ban đầu
  digitalWrite(RELAY_LIGHT_PIN, HIGH);
  digitalWrite(RELAY_FAN_PIN, LOW);
}

void loop() {
  // Kiểm tra nút bấm và điều chỉnh ngưỡng
  static unsigned long lastButtonPress = 0;
  unsigned long debounceDelay = 200; // Thời gian debounce cho nút bấm

  if (millis() - lastButtonPress > debounceDelay) {
    if (digitalRead(BUTTON1_PIN) == LOW) {
      switch (currentState) {
        case STATE_SET_TEMP_THRESHOLD:
          tempThreshold += 0.5;
          break;
        case STATE_SET_LUX_THRESHOLD:
          luxThreshold += 0.5;
          break;
        default:
          break;
      }
      lastButtonPress = millis();
    }
    if (digitalRead(BUTTON2_PIN) == LOW) {
      switch (currentState) {
        case STATE_SET_TEMP_THRESHOLD:
          tempThreshold -= 0.5;
          break;
        case STATE_SET_LUX_THRESHOLD:
          luxThreshold -= 0.5;
          break;
        default:
          break;
      }
      lastButtonPress = millis();
    }
    if (digitalRead(BUTTON3_PIN) == LOW) {
      switch (currentState) {
        case STATE_SET_TEMP_THRESHOLD:
          currentState = STATE_SET_LUX_THRESHOLD;
          break;
        case STATE_SET_LUX_THRESHOLD:
          currentState = STATE_DISPLAY;
          break;
      }
      lastButtonPress = millis();
    }
    if (digitalRead(BUTTON4_PIN) == LOW) {

      if (currentState == STATE_DISPLAY) {
        digitalWrite(RELAY_LIGHT_PIN, HIGH);
        digitalWrite(RELAY_FAN_PIN, LOW);
        currentState = STATE_SET_TEMP_THRESHOLD;
        systemPaused = true; // Tạm dừng hệ thống
      } else {
        currentState = STATE_DISPLAY;
        systemPaused = false; // Khởi động lại hệ thống
      }
      lastButtonPress = millis();
    }
  }

  if (!systemPaused) {
    // Đọc nhiệt độ từ DS18B20
    sensors.requestTemperatures();
    float temperature = sensors.getTempCByIndex(0);
    // Đọc ánh sáng từ BH1750
    float lux = lightMeter.readLightLevel();

    // Điều khiển relay
    if (temperature > tempThreshold) {
      digitalWrite(RELAY_FAN_PIN, HIGH); // Bật quạt
    } else {
      digitalWrite(RELAY_FAN_PIN, LOW); // Tắt quạt
    }

    if (lux < luxThreshold) {
      digitalWrite(RELAY_LIGHT_PIN, LOW); // Bật đèn
    } else {
      digitalWrite(RELAY_LIGHT_PIN, HIGH); // Tắt đèn
    }

    // Hiển thị thông tin lên LCD
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("T:");
    lcd.print(temperature);
    lcd.print("C");

    lcd.setCursor(0, 1);
    lcd.print("Lux:");
    lcd.print(lux);
  } else {
    // Hiển thị thông tin cài đặt ngưỡng lên LCD
    lcd.clear();
    switch (currentState) {
      case STATE_SET_TEMP_THRESHOLD:
        lcd.setCursor(0, 0);
        lcd.print("Set Temp Thresh:");
        lcd.setCursor(0, 1);
        lcd.print(tempThreshold);
        lcd.print(" C");
        break;
      case STATE_SET_LUX_THRESHOLD:
        lcd.setCursor(0, 0);
        lcd.print("Set Lux Thresh:");
        lcd.setCursor(0, 1);
        lcd.print(luxThreshold);
        lcd.print(" Lux");
        break;
      default:
        break;
    }
  }

  delay(500); // Chờ trước khi cập nhật lại
}