//////////////////////////////////////////////
//            RemoteXY SETUP               //
//////////////////////////////////////////////

#define REMOTEXY_MODE__ESP8266_HARDSERIAL_POINT

#define REMOTEXY_SERIAL Serial
#define REMOTEXY_SERIAL_SPEED 115200
#define REMOTEXY_WIFI_SSID "Arduino"
#define REMOTEXY_WIFI_PASSWORD "12345678"
#define REMOTEXY_SERVER_PORT 6377

#include <RemoteXY.h>

#pragma pack(push, 1)
uint8_t const PROGMEM RemoteXY_CONF_PROGMEM[] = {
  255,1,0,7,0,66,0,19,0,0,0,0,31,1,106,200,1,1,5,0,
  2,41,22,25,16,0,40,26,31,31,79,78,0,79,70,70,0,67,17,59,
  73,9,78,40,26,2,70,44,95,17,17,16,31,1,0,70,44,119,17,17,
  16,31,204,0,70,44,143,17,17,16,31,19,0
};

struct {
  uint8_t Gas_switch_02;  
  float Gas_Value;

  uint8_t Gas_led_Red;
  uint8_t Gas_led_Blue;
  uint8_t Gas_led_yellow;

  uint8_t connect_flag;
} RemoteXY;
#pragma pack(pop)

#define PIN_GAS_SWITCH_02 9

//////////////////////////////////////////////
//             LIBRARIES                  //
//////////////////////////////////////////////

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Servo.h>

//////////////////////////////////////////////
//             HARDWARE PINS              //
//////////////////////////////////////////////

#define flamePin 2
#define gasPin A0
#define buzzer 8

#define yellowLED 13
#define blueLED 12
#define redLED 11

#define fanPin 7
#define servoPin 6

Servo myServo;
LiquidCrystal_I2C lcd(0x27, 16, 2);

int yellowLevel = 350;
int blueLevel   = 450;
int redLevel    = 550;

bool servoState = false;   // 🔥 internal safety state

//////////////////////////////////////////////
//                SETUP                   //
//////////////////////////////////////////////

void setup() {

  RemoteXY_Init();

  pinMode(flamePin, INPUT);
  pinMode(buzzer, OUTPUT);

  pinMode(yellowLED, OUTPUT);
  pinMode(blueLED, OUTPUT);
  pinMode(redLED, OUTPUT);

  pinMode(fanPin, OUTPUT);

  myServo.attach(servoPin);
  myServo.write(0);

  lcd.init();
  lcd.backlight();

  lcd.setCursor(0,0);
  lcd.print("Smart System");
  delay(2000);
  lcd.clear();
}

//////////////////////////////////////////////
//                LOOP                    //
//////////////////////////////////////////////

void loop() {

  RemoteXY_Handler();

  int flame = digitalRead(flamePin);
  int gas = analogRead(gasPin);

  RemoteXY.Gas_Value = gas;

  //////////////////////////////////////////
  // SERVO SAFETY LOGIC (AUTO CONTROL)
  //////////////////////////////////////////

  if (flame == LOW || gas >= yellowLevel) {
    servoState = true;
  } else {
    servoState = false;
  }

  if (servoState) {
    myServo.write(180);
    RemoteXY.Gas_switch_02 = 0;   // 📱 status ON
  } else {
    myServo.write(0);
    RemoteXY.Gas_switch_02 = 1;   // 📱 status OFF
  }

  //////////////////////////////////////////
  // LCD DISPLAY
  //////////////////////////////////////////

  lcd.setCursor(0, 0);

  if (flame == LOW) lcd.print("!! FIRE ALERT !!");
  else if (gas >= redLevel) lcd.print("STATUS: DANGER ");
  else if (gas >= blueLevel) lcd.print("STATUS: HIGH   ");
  else if (gas >= yellowLevel) lcd.print("STATUS: WARNING");
  else lcd.print("STATUS: SAFE   ");

  lcd.setCursor(0, 1);
  lcd.print("GAS: ");
  lcd.print(gas);
  lcd.print("    ");

  //////////////////////////////////////////
  // RESET LED
  //////////////////////////////////////////

  digitalWrite(yellowLED, LOW);
  digitalWrite(blueLED, LOW);
  digitalWrite(redLED, LOW);

  //////////////////////////////////////////
  // FIRE PRIORITY (HIGHEST)
  //////////////////////////////////////////

  if (flame == LOW) {

    lcd.setCursor(0,0);
    lcd.print("FIRE DETECTED!! ");

    digitalWrite(redLED, HIGH);
    digitalWrite(buzzer, HIGH);
    digitalWrite(fanPin, HIGH);

    RemoteXY.Gas_led_Red = 1;
    RemoteXY.Gas_led_Blue = 0;
    RemoteXY.Gas_led_yellow = 0;

    delay(150);
    digitalWrite(buzzer, LOW);
    delay(150);

    return;
  }

  //////////////////////////////////////////
  // GAS LOGIC
  //////////////////////////////////////////

  if (gas >= redLevel) {

    digitalWrite(yellowLED, HIGH);
    digitalWrite(blueLED, HIGH);
    digitalWrite(redLED, HIGH);

    digitalWrite(buzzer, HIGH);
    delay(100);
    digitalWrite(buzzer, LOW);
    delay(100);

    RemoteXY.Gas_led_Red = 1;
    RemoteXY.Gas_led_Blue = 1;
    RemoteXY.Gas_led_yellow = 1;
  }

  else if (gas >= blueLevel) {

    digitalWrite(yellowLED, HIGH);
    digitalWrite(blueLED, HIGH);

    digitalWrite(buzzer, HIGH);
    delay(200);
    digitalWrite(buzzer, LOW);
    delay(200);

    RemoteXY.Gas_led_Red = 0;
    RemoteXY.Gas_led_Blue = 1;
    RemoteXY.Gas_led_yellow = 1;
  }

  else if (gas >= yellowLevel) {

    digitalWrite(yellowLED, HIGH);

    digitalWrite(buzzer, HIGH);
    delay(300);
    digitalWrite(buzzer, LOW);
    delay(300);

    RemoteXY.Gas_led_Red = 0;
    RemoteXY.Gas_led_Blue = 0;
    RemoteXY.Gas_led_yellow = 1;
  }

  else {

    digitalWrite(buzzer, LOW);

    RemoteXY.Gas_led_Red = 0;
    RemoteXY.Gas_led_Blue = 0;
    RemoteXY.Gas_led_yellow = 0;
  }

  //////////////////////////////////////////
  // FAN CONTROL
  //////////////////////////////////////////

  digitalWrite(fanPin, (gas >= yellowLevel) ? HIGH : LOW);

  delay(50);
}