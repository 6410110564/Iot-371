#include <esp_sleep.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

#define SLEEP_TIME 5000000 //5seconds
#define LED 14
#define SLEEP_PIN 12
#define BUZZER 18

bool buzzerActive = false;
bool ledActive = false;


unsigned long debounceDelay = 10;
int lastButtonState3 = HIGH;  // เพิ่มตัวแปร static
unsigned long last_time3 = 0;

LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
    Serial.begin(115200);
    // Wire.begin();
    // lcd.begin(16, 2);
    // lcd.backlight();
    // lcd.setCursor(2, 1); // ไปที่ตัวอักษรที่ 7 แถวที่ 2
    // lcd.print("NOW:  1200 ml.");
    pinMode(LED, OUTPUT);
    pinMode(SLEEP_PIN, INPUT_PULLUP);
    pinMode(BUZZER, OUTPUT);
    // Serial.println("hello");
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
    // Serial.println(digitalRead(SLEEP_PIN));
    // attachInterrupt(digitalPinToInterrupt(SLEEP_PIN), sleep, FALLING);
}

void loop() { 
    check_sleep();
}

void wakeUp() {
    // ตื่นขึ้นมาและทำงานที่นี่
    // LED ติด
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
        // if(digitalRead(SLEEP_PIN) == LOW){
        //     // attachInterrupt(digitalPinToInterrupt(SLEEP_PIN), sleep, FALLING);
        //     lcd.noDisplay(); // ปิดการแสดงตัวอักษร
        //     lcd.noBacklight(); // ปิดไฟแบล็กไลค์
        //     sleep();
        // }
        // Serial.println(digitalRead(SLEEP_PIN));
    }
}

void sleep(){
    esp_sleep_enable_timer_wakeup(SLEEP_TIME);
    esp_deep_sleep_start(); 
}

void check_sleep(){
    int sleepstate = digitalRead(SLEEP_PIN);
    Serial.println(sleepstate);
    if (sleepstate != lastButtonState3){
        lastButtonState3 = sleepstate;
        if (millis() - last_time3 > debounceDelay){
            last_time3 = millis();
            if (sleepstate == LOW){
                lcd.noDisplay(); 
                lcd.noBacklight(); 
                sleep(); 
            }
        }
    }
    
        
        
     
}


