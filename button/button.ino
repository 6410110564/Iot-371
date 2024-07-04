// const char* vol[] = {"1000","1500","2000","2500","3000","3500"};
// #define VOL_PIN 13


// int selectedValue = 0;
// unsigned long last_time = 0;
// unsigned long debounce_delay = 50;  // ลด debounce delay ลงมาเล็กน้อย
// int lastButtonState = HIGH;  // เพิ่มตัวแปร static

// // void setup() {
// //     Serial.begin(115200);
// //     pinMode(VOL_PIN, INPUT_PULLUP);
// // }

// void goal_vol() {
//     int buttonState = digitalRead(VOL_PIN);
//     if (buttonState != lastButtonState) {
//         // ตรวจสอบ debounce  เพื่อตรวจสอบว่ามีการเปลี่ยนแปลงของสถานะปุ่ม.
//         // Serial.println(lastButtonState); //lastรอบแรกเป็น 1 (กำหนดไว้)รอบสองเป็น 0 
//         lastButtonState = buttonState;
//         // Serial.println(lastButtonState); //lastรอบแรกเป็น 0 (กดอยู่) รอบสองเป็น 1 
//         if (millis() - last_time > debounce_delay) {
//             // เพิ่มตรวจสอบว่าปุ่มตอนนี้กดหรือปล่อย
//             last_time = millis();
//             if (buttonState == LOW) {
//                 Serial.println(vol[selectedValue]);
//                 selectedValue = (selectedValue + 1) % (sizeof(vol) / sizeof(vol[0]));
//             }  
//         }
//     }   
// }


//ปุ่มสุดท้ายให้สรุปค่าแล้วดับ ตื่นขึ้นนมาเริ่มนับเลขด้วยปุ่มเดิม ยังไม่สั่งเคลียร์ค่า
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <esp_sleep.h>
#define FINISH_PIN 35
// int count = 0;

volatile bool sleepFlag = false;
volatile int count = 0;
LiquidCrystal_I2C lcd(0x27, 16, 2);
void setup(){
    Serial.begin(115200);
    Wire.begin();
    lcd.begin(16, 2);
    lcd.backlight();
    pinMode(FINISH_PIN, INPUT);
    attachInterrupt(digitalPinToInterrupt(FINISH_PIN),buttonISR,CHANGE);
}

void buttonISR(){
    sleepFlag = true;
}

void loop(){ 
    delay(1000);
    count++;
    Serial.println(count);

    if (sleepFlag) {
    //อัปค่าขึ้น cloud
    // Display the count value
    Serial.println("Count: " + String(count));
    
    // Enter sleep mode
    lcd.noDisplay(); 
    lcd.noBacklight(); 
    finished();
  }
}

void finished(){
    esp_sleep_enable_ext0_wakeup((gpio_num_t)FINISH_PIN, LOW);
    Serial.println("go sleep....");
    esp_deep_sleep_start();

}