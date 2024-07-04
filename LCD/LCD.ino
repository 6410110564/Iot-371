#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Preferences.h>
#include <esp_sleep.h>
#include <HX711.h>
#include <HTTPClient.h>
#include <Wifi.h>

const char *host = "script.google.com";
const int httpsPort = 443;
String GAS_ID = "AKfycbwsvf5lBq4OK1Vamcd5D-I7TYoPigxLhRQuoEQ-NpVh-taSYkeIHrpB0OAq95a0XTSo"; //--> spreadsheet script ID

const char *ssid = "firdaws.sss";
const char *password = "48694869";

#define GOAL_PIN 36
#define SIGN_PIN 39
#define SLEEP_PIN 34
#define FINISH_PIN 35

#define POTEN_PIN 32

#define SLEEP_TIME 10000000 // 5seconds
//unsigned long  SLEEP_TIME = 3600000000UL;
#define LED 14
#define BUZZER 18

float calibration_factor = 217048.00;
#define zero_factor 8602212
#define DOUT 25
#define CLK 26

const char *vol[] = {"1000", "1500", "2000", "2500", "3000", "3500"};

byte sound_sign[8] = {
    B11111,
    B11001,
    B11011,
    B11011,
    B11011,
    B10011,
    B10011,
    B11111};
byte led_sign[8] = {
    B11111,
    B10001,
    B10001,
    B10001,
    B11011,
    B11011,
    B11011,
    B11111};

int mode = 1;

int selectedvol = 0;
int selectedsign = 0;
unsigned long last_time1 = 0;
unsigned long last_time2 = 0;
unsigned long last_time3 = 0;
unsigned long previousMillis = 0;
unsigned long delaytosleep = 0;
unsigned long debounce_delay = 50; // ลด debounce delay ลงมาเล็กน้อย
int lastButtonState1 = HIGH;       // เพิ่มตัวแปร static
int lastButtonState2 = HIGH;       // เพิ่มตัวแปร static
int lastButtonState3 = HIGH;       // เพิ่มตัวแปร static

bool buzzerState = false;
bool ledState = false;

bool wakeup = false;
bool setgoal = false;
bool quite_yet = false;

int before;
int after;
int result;
int sum = 0;
float offset = 0;
float get_units_g();

volatile bool sleepFlag = false;

// Set the LCD address to 0x27 or 0x3F for a 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x27, 16, 2);
Preferences preferences;
HX711 scale(DOUT, CLK);

void setup()
{
  Serial.begin(115200);
  // WiFi.begin(ssid, password);
  // while (WiFi.status() != WL_CONNECTED)
  // {
  //   delay(1000);
  //   Serial.println("Connecting to WiFi...");
  // }
  // Serial.println("Connected to WiFi");

  
  Wire.begin();
  lcd.begin(16, 2);
  preferences.begin("my-app", false);
  // Turn on the blacklight and print a message.
  lcd.backlight();

  scale.set_scale(calibration_factor);
  scale.set_offset(zero_factor);
  sum = preferences.getInt("sum", 0);

  lcd.createChar(0, sound_sign);
  lcd.createChar(1, led_sign);
  lcd.setCursor(2, 0); // ไปที่ตัวอักษรที่ 0 แถวที่ 1

  selectedvol = preferences.getInt("selectedvol", 0);
  lcd.setCursor(2, 0);
  lcd.print("GOAL: " + String(vol[selectedvol]) + " ml.");

  selectedsign = preferences.getInt("selectedsign", 0);

  lcd.setCursor(2, 1); // ไปที่ตัวอักษรที่ 7 แถวที่ 2
  lcd.print("NOW :      ml.");
  lcd.setCursor(8, 1);
  lcd.print(sum);

  pinMode(GOAL_PIN, INPUT_PULLUP);
  pinMode(SIGN_PIN, INPUT_PULLUP);
  pinMode(LED, OUTPUT);
  pinMode(SLEEP_PIN, INPUT_PULLUP);
  pinMode(BUZZER, OUTPUT);

  pinMode(FINISH_PIN, INPUT_PULLUP);
  attachInterrupt(
      digitalPinToInterrupt(FINISH_PIN), []()
      { sleepFlag = true; },
      FALLING);
  check_sign();
  check_wakeup();
}
void loop()
{
  if (mode == 1)
  { // เริ่มการทำงาน mode1 เลือก goalได้
    goal_vol();
    sign();
  }
  else
  { // เมื่อตื่นจะเป็น mode0 เลือก goal ไม่ได้แต่จะปิดเตือนแทน
    quite();
    if (quite_yet)
    {
      sign();
    }
  }
  check_sleep();
  if (wakeup)
  {
    if (selectedsign == 0)
    {
      both_show();
    }
    else if (selectedsign == 1)
    {
      buzzer_show();
    }
    else
    {
      led_show();
    }
  }

  if (sleepFlag)
  {
    after = get_units_g() + offset;
    Serial.printf("AFTER: %d g\n", after);

    result = before - after;
    if (before < after)
    {
      result = 0;
    }

    Serial.printf("result = %d g\n", result);
    sum = sum + result;

    // delay(1000);
    Serial.printf("sum = %d g\n", sum);
    preferences.putInt("sum", sum);

    // เรียกฟังก์ชัน cloud
    sendDataToGoogleSheet();
    // ต้องอ่าน น้ำรอบนั้นก่อนแล้วค่อยอัป*****


    // Serial.println("go sleep");
    lcd.setCursor(2, 1); // ไปที่ตัวอักษรที่ 7 แถวที่ 2
    lcd.print("NOW :      ml.");
    lcd.setCursor(8, 1);
    lcd.print(sum);
    Serial.flush();
    // Display the count value
    Serial.println("result before to reset: " + String(sum));
    delay(5000);
    // Enter sleep mode
    lcd.noDisplay();
    lcd.noBacklight();
    preferences.clear();
    finished();
  }
}

void sendDataToGoogleSheet()
{
  float percentage = (sum / atof(vol[selectedvol])) * 100;
  String jsonPayload = "{\"Goal\":\"" + String(vol[selectedvol]) + "\",\"Sum\":\"" + String(sum) + "\",\"Percentage\":\"" + String(percentage) + "%\"}";

  HTTPClient http;
  const char *scriptUrl = "https://script.google.com/macros/s/AKfycbwsvf5lBq4OK1Vamcd5D-I7TYoPigxLhRQuoEQ-NpVh-taSYkeIHrpB0OAq95a0XTSo/exec";
  http.begin(scriptUrl);
  http.addHeader("Content-Type", "application/json");
  int httpResponseCode = http.POST(jsonPayload);
  if (httpResponseCode > 0)
  {
    Serial.print("HTTP Response code: ");
    Serial.println(httpResponseCode);
    if (percentage >= 100)
    {
      Serial.println("Success: Goal achieved!");
    }
    else
    {
      Serial.println("Fail: Goal not achieved!");
    }
  }
  else
  {
    Serial.print("Error during HTTP POST request: ");
    Serial.println(httpResponseCode);
  }
  http.end();
}

void quite()
{
  int buttonState = digitalRead(GOAL_PIN);
  if (buttonState != lastButtonState1)
  {
    // ตรวจสอบ debounce  เพื่อตรวจสอบว่ามีการเปลี่ยนแปลงของสถานะปุ่ม.
    // Serial.println(lastButtonState); //รอบแรกเป็น 1 (กำหนดไว้)รอบสองเป็น 0
    lastButtonState1 = buttonState;
    // Serial.println(lastButtonState); //รอบแรกเป็น 0 (กดอยู่) รอบสองเป็น 1
    if (millis() - last_time1 > debounce_delay)
    {
      // เพิ่มตรวจสอบว่าปุ่มตอนนี้กดหรือปล่อย
      last_time1 = millis();
      if (buttonState == LOW)
      {
        Serial.println(GOAL_PIN);
        wakeup = false;
        digitalWrite(LED, LOW);
        noTone(BUZZER);
        quite_yet = true;
      }
    }
  }
}

void finished()
{
  esp_sleep_enable_ext0_wakeup((gpio_num_t)FINISH_PIN, LOW);
  Serial.println("go sleep....");
  esp_deep_sleep_start();
}

void goal_vol()
{
  int buttonState = digitalRead(GOAL_PIN);
  if (buttonState != lastButtonState1)
  {
    // ตรวจสอบ debounce  เพื่อตรวจสอบว่ามีการเปลี่ยนแปลงของสถานะปุ่ม.
    // Serial.println(lastButtonState); //รอบแรกเป็น 1 (กำหนดไว้)รอบสองเป็น 0
    lastButtonState1 = buttonState;
    // Serial.println(lastButtonState); //รอบแรกเป็น 0 (กดอยู่) รอบสองเป็น 1
    if (millis() - last_time1 > debounce_delay)
    {
      // เพิ่มตรวจสอบว่าปุ่มตอนนี้กดหรือปล่อย
      last_time1 = millis();
      if (buttonState == LOW)
      {
        // Serial.println(vol[selectedvol]);
        lcd.setCursor(2, 0); // ไปที่ตัวอักษรที่ 0 แถวที่ 1
        selectedvol = (selectedvol + 1) % (sizeof(vol) / sizeof(vol[0]));
        lcd.print("GOAL: " + String(vol[selectedvol]) + " ml.");

        Serial.print("goal: ");
        Serial.println(vol[selectedvol]);

        preferences.putInt("selectedvol", selectedvol);
      }
    }
  }
}

void sign()
{
  int signbutton = digitalRead(SIGN_PIN);
  if (signbutton != lastButtonState2)
  {
    // ตรวจสอบ debounce  เพื่อตรวจสอบว่ามีการเปลี่ยนแปลงของสถานะปุ่ม.
    // Serial.println(lastButtonState); //รอบแรกเป็น 1 (กำหนดไว้)รอบสองเป็น 0
    lastButtonState2 = signbutton;
    // Serial.println(lastButtonState); //รอบแรกเป็น 0 (กดอยู่) รอบสองเป็น 1
    if (millis() - last_time2 > debounce_delay)
    {
      // เพิ่มตรวจสอบว่าปุ่มตอนนี้กดหรือปล่อย
      last_time2 = millis();
      if (signbutton == LOW)
      {

        selectedsign < 2 ? selectedsign++ : selectedsign = 0;
        Serial.print("sign:");
        Serial.println(selectedsign);

        preferences.putInt("selectedsign", selectedsign);
        check_sign();
      }
    }
  }
}

void check_sign()
{
  switch (selectedsign)
  {
  case 0:
    lcd.setCursor(0, 0);
    lcd.write(byte(0)); // Sound
    lcd.setCursor(0, 1);
    lcd.write(byte(1)); // LED
    break;
  case 1:
    lcd.setCursor(0, 1);
    lcd.print(" ");
    lcd.setCursor(0, 0);
    lcd.write(byte(0)); // Sound
    break;
  case 2:
    lcd.setCursor(0, 0);
    lcd.print(" ");
    lcd.setCursor(0, 1);
    lcd.write(byte(1)); // LED
    break;
  default:
    lcd.setCursor(0, 0);
    lcd.write(byte(0)); // Sound
    lcd.setCursor(0, 1);
    lcd.write(byte(1)); // LED
    break;
  }
}

void both_show()
{
  if (millis() - previousMillis >= 400)
  {
    previousMillis = millis(); // อัพเดทเวลาล่าสุด
    // เปลี่ยนสถานะของ LED
    ledState = !ledState;
    buzzerState = !buzzerState;
    if (buzzerState)
    {
      digitalWrite(LED, ledState);
      tone(BUZZER,70);
    }
    else
    {
      digitalWrite(LED, ledState);
      noTone(BUZZER);
    }
  }
}

void buzzer_show()
{
  if (millis() - previousMillis >= 400)
  {
    previousMillis = millis(); // อัพเดทเวลาล่าสุด
    // เปลี่ยนสถานะของ LED
    buzzerState = !buzzerState;
    if (buzzerState)
    {
      tone(BUZZER, 70);
    }
    else
    {
      noTone(BUZZER);
    }
  }
}

void led_show()
{
  if (millis() - previousMillis >= 400)
  {
    previousMillis = millis(); // อัพเดทเวลาล่าสุด
    // เปลี่ยนสถานะของ LED
    ledState = !ledState;
    digitalWrite(LED, ledState);
  }
}

void sleep()
{
  // esp_sleep_enable_timer_wakeup(1000000ULL * SLEEP_TIME);
  esp_sleep_enable_timer_wakeup(SLEEP_TIME);
  esp_deep_sleep_start();
}

void check_sleep()
{
  int sleepstate = digitalRead(SLEEP_PIN);
  // Serial.println(sleepstate);
  if (sleepstate != lastButtonState3)
  {
    wakeup = false;
    lastButtonState3 = sleepstate;
    if (millis() - last_time3 > debounce_delay)
    {
      last_time3 = millis();
      if (sleepstate == LOW)
      {
        wakeup = false;
        after = get_units_g() + offset;
        Serial.printf("AFTER: %d g\n", after);

        result = before - after;
        if (before < after)
        {
          result = 0;
        }

        Serial.printf("result = %d g\n", result);
        sum = sum + result;

        // delay(1000);
        Serial.printf("sum = %d g\n", sum);
        preferences.putInt("sum", sum);
        // delay(1000);

        // เรียกฟังก์ชัน cloud
        sendDataToGoogleSheet();

        Serial.println("go sleep");
        lcd.setCursor(2, 1); // ไปที่ตัวอักษรที่ 7 แถวที่ 2
        lcd.print("NOW :      ml.");
        lcd.setCursor(8, 1);
        lcd.print(sum);
        Serial.flush();
        delay(5000);

        lcd.noDisplay();
        lcd.noBacklight();
        sleep();
      }
    }
  }
}

void check_wakeup()
{
  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER)
  {
    // ตื่นขึ้นมาจากโหมด sleep ด้วยตัวจับเวลา
    // wakeUp();
    mode < 1 ? mode++ : mode = 0;
    Serial.print("mode: ");
    Serial.println(mode);
    read_weight();
    wakeup = true;
  }
  else
  {
    // Serial.println(digitalRead(SLEEP_PIN));
  }
}

float get_units_g()
{
  return (scale.get_units() * 0.453592*1000);
}

void read_weight()
{
  before = get_units_g() + offset;
  Serial.printf("BEFORE: %d g\n", before);
}