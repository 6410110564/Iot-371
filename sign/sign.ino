#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Preferences.h>
#include <esp_sleep.h>

#define SIGN_PIN 27
#define LED 14
#define SLEEP_PIN 26
#define BUZZER 18
#define SLEEP_TIME 5000000 //5seconds

byte sound_sign[8] = { 
  B11111,
  B11001,
  B11011,
  B11011,
  B11011,
  B10011,
  B10011,
  B11111
};
byte led_sign[8] = {
  B11111,
  B10001,
  B10001,
  B10001,
  B11011,
  B11011,
  B11011,
  B11111
};


int selectedsign = 0;
unsigned long last_time = 0;
unsigned long debounce_delay = 50;  // ลด debounce delay ลงมาเล็กน้อย
int lastButtonState = HIGH;  // เพิ่มตัวแปร static

bool buzzerActive = false;
bool ledActive = false;

// Set the LCD address to 0x27 or 0x3F for a 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x27, 16, 2);
Preferences preferences;

void setup()
{
  Serial.begin(115200);
  Wire.begin();
  lcd.begin(16, 2);

  // Turn on the blacklight and print a message.
  lcd.backlight();
  lcd.createChar(0, sound_sign);
  lcd.createChar(1, led_sign);
  pinMode(SIGN_PIN, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
  pinMode(SLEEP_PIN, INPUT_PULLUP);
  pinMode(BUZZER, OUTPUT);

  preferences.begin("my-app", false);
  selectedsign = preferences.getInt("selectedsign", 0);
  check_sign();
  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER) {
        // ตื่นขึ้นมาจากโหมด sleep ด้วยตัวจับเวลา
        wakeUp();
  } else {
    // เริ่มต้นจาก LED ดับและบัสเซอร์เงียบ
    // digitalWrite(LED, LOW);
    // noTone(BUZZER);
    Serial.println(digitalRead(SLEEP_PIN));
    // ตั้งค่าการตื่นในอนาคตจากโหมด sleep
    esp_sleep_enable_timer_wakeup(SLEEP_TIME);
  }
}


void loop() {
  sign();
  check_sleep();
}

void sign(){
  int signbutton = digitalRead(SIGN_PIN);

  // Serial.println(selectedsign);
  
    if (signbutton != lastButtonState) {
      // Serial.println(lastButtonState); //รอบแรกเป็น 1 (กำหนดไว้)รอบสองเป็น 0 
      lastButtonState = signbutton;
      // Serial.println(lastButtonState); //รอบแรกเป็น 0 (กดอยู่) รอบสองเป็น 1 
      if (millis() - last_time > debounce_delay) {
        last_time = millis();
        if (signbutton == LOW) {    
          selectedsign < 2? selectedsign++ : selectedsign = 0;
          Serial.println(selectedsign);
          preferences.putInt("selectedsign", selectedsign);
          check_sign();
        }  
      }
    }   
}

void check_sign(){
  switch (selectedsign) {
    case 0:
      lcd.setCursor(0, 0);
      lcd.write(byte(0));  // Sound
      lcd.setCursor(0, 1);
      lcd.write(byte(1));  // LED
      break;
    case 1:
      lcd.setCursor(0, 1);
      lcd.print(" ");
      lcd.setCursor(0, 0);
      lcd.write(byte(0));  // Sound
      break;
    case 2:
      lcd.setCursor(0, 0);
      lcd.print(" ");
      lcd.setCursor(0, 1);
      lcd.write(byte(1));  // LED
      break;
    default:
      lcd.setCursor(0, 0);
      lcd.write(byte(0));  // Sound
      lcd.setCursor(0, 1);
      lcd.write(byte(1));  // LED
      break;
  }
}

void wakeUp() {
    // ตื่นขึ้นมาและทำงานที่นี่
    // LED ติด
  if(selectedsign == 0){
    for(int i=0; i<1000; i++){
      digitalWrite(LED, HIGH);
      // บัสเซอร์ดัง
      tone(BUZZER, 20);
      delay(200);
      noTone(BUZZER);
      // LED ดับ
      digitalWrite(LED, LOW);
      delay(500);
      check_sleep();
        // Serial.println(digitalRead(SLEEP_PIN));
    }
  }
  else if(selectedsign == 1){
    for(int i=0; i<1000; i++){
      // digitalWrite(LED, HIGH);
      // บัสเซอร์ดัง
      tone(BUZZER, 20);
      delay(200);
      noTone(BUZZER);
      // LED ดับ
      // digitalWrite(LED, LOW);
      delay(500);
      check_sleep();
    }
  }
  else{
    for(int i=0; i<1000; i++){
      digitalWrite(LED, HIGH);
      // บัสเซอร์ดัง
      // tone(BUZZER, 20);
      delay(200);
      // noTone(BUZZER);
      // LED ดับ
      digitalWrite(LED, LOW);
      delay(500);
      check_sleep();
    }
  }
}

void check_sleep(){
  if(digitalRead(SLEEP_PIN) == LOW){
    Serial.println(digitalRead(SLEEP_PIN));
    while(!digitalRead(SLEEP_PIN)){}
    // attachInterrupt(digitalPinToInterrupt(SLEEP_PIN), sleep, FALLING);
    lcd.noDisplay(); // ปิดการแสดงตัวอักษร
    lcd.noBacklight(); // ปิดไฟแบล็กไลค์
    sleep();
  }
}

void sleep(){
  esp_sleep_enable_timer_wakeup(SLEEP_TIME);
  esp_deep_sleep_start(); 
}