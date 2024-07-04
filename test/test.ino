#include <HX711.h>
#include <esp_sleep.h>
#include <Preferences.h>
//8600879
//178127.00

float calibration_factor =178127.00; 
#define zero_factor 8600879
#define DOUT  25
#define CLK   26
#define DEC_POINT  0

#define SLEEP_PIN 34
#define SLEEP_TIME 5000000 //5seconds

int data1;
int data2;
int result;
int result1;
float offset=0;
float get_units_kg();
unsigned long debounceDelay = 50;
int lastButtonState3 = HIGH;  // เพิ่มตัวแปร static
unsigned long last_time3 = 0;
unsigned long lastDebounceTime = 0;


HX711 scale(DOUT, CLK);
Preferences preferences;

void setup() 
{
  Serial.begin(115200);
  preferences.begin("my-app", false);
  Serial.println("Load Cell");
  scale.set_scale(calibration_factor); 
  scale.set_offset(zero_factor);
  result1 = preferences.getInt("result1", 0);
  pinMode(SLEEP_PIN, INPUT_PULLUP); 

  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER) {
    // ตื่นขึ้นมาจากโหมด sleep ด้วยตัวจับเวลา
    wakeUp();
  } 

}
void loop() 
{ 
  // kg();
  check_sleep();
}
float get_units_kg()
{
  return(scale.get_units()*0.453592*1000);
}

void kg(){
  data1 = get_units_kg() + offset;
  Serial.print("Reading1: ");
  Serial.print(data1);
  Serial.println(" g");
  
}

void wakeUp() {
  kg();
  // check_sleep();
}

void check_sleep(){
  int sleepstate = digitalRead(SLEEP_PIN);
  if (sleepstate != lastButtonState3){
    lastButtonState3 = sleepstate;
    if (millis() - last_time3 > debounceDelay){
      lastDebounceTime = millis();
      if (sleepstate == LOW){
        data2 = get_units_kg() + offset;
        Serial.print("Reading2: ");
        Serial.print(data2);
        Serial.println(" g");

        result = data1-data2;
        // delay(1000);
        Serial.printf("result = %d\n",result);
        result1 = result1 + result;
        delay(1000);
        Serial.printf("result_real = %d\n",result1);
        preferences.putInt("result1", result1);
        delay(3000);

        Serial.println("go sleep");
        Serial.flush();
        sleep(); 
      }
    }
  }    
}

void sleep(){
    esp_sleep_enable_timer_wakeup(SLEEP_TIME);
    esp_deep_sleep_start(); 
}