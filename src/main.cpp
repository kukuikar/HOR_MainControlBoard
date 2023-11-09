#include <SoftwareSerial.h>
#include <GParser.h>
#include <AsyncStream.h>
#include <ServoSmooth.h>
#include <GyverOLED.h>
#include <EncButton.h>
#include <Arduino.h>
class LIFT
{
public:
  LIFT(int SERVO_PIN, int LIMIT_UP_PIN, int LIMIT_BOT_PIN)
  {
    _srv_pin = SERVO_PIN;
    _limit_up_pin = LIMIT_UP_PIN;
    _limit_bot_pin = LIMIT_BOT_PIN;
    // pinMode(SERVO_PIN, OUTPUT);
    // pinMode(LIMIT_UP_PIN, INPUT_PULLUP);
    // pinMode(LIMIT_BOT_PIN, INPUT_PULLUP);
    _servo.attach(_srv_pin);
  }
  void drive(int ang)
  {
    _servo.write(ang * k);
  }
  float accel = 0.25;
  float k = 1.0;

private:
  int _srv_pin;
  int _limit_up_pin;
  int _limit_bot_pin;
  ServoSmooth _servo;
};
// Serial1 - Bridge
//  TX1 = 18
//  RX1 = 19

// Serial2 - Spreader
//  TX2 = 16
//  RX2 = 17

// Serial3 - MiniCranes
//  TX3 = 14
//  RX3 = 15

// SoftSerial - Reciever
//  TX = 3
//  RX = 2

#define TERMINATOR ';'

///////////////////
/// PINS DEFINE ///
///////////////////
int LIFT_PINS[6] = {40, 41, 42, 43, 44, 45};
int LIFT_LIMIT_PINS[2][6] =
{
  {28, 30, 32, 34, 36, 38}, /*up limit*/
  {29, 31, 33, 35, 37, 39}, /*bottom limit*/
};

float LIFT_K[6] = {1.0, 1.0, 1.0, 1.0, 1.0, 1.0};

#define TRANS_Rx_PIN 2
#define TRANS_Tx_PIN 3
#define MANUAL_LIFT_PIN 46
#define LIFTING_LED_PIN 4
#define servoManualGimbalPin A0

/////////////////////
/// COMMUNICATION ///
/////////////////////
SoftwareSerial Reciever(TRANS_Rx_PIN, TRANS_Tx_PIN);
AsyncStream<25> Serial_Rx(&Reciever, TERMINATOR);
EncButton encoderServo(25, 26, 27); // 25-26 - encoder, 27 - button
GyverOLED<SSD1306_128x64, OLED_BUFFER, OLED_SPI, 22, 23, 24> oled;

//////////////
/// TIMERS ///
//////////////
uint32_t tmr = millis();
uint32_t tmr_oled = millis();
int tmr_oled_inerval = 200;

/////////////////
/// VARIABLES ///
/////////////////
int servoNumManual = 6;
int pointer = 0;
bool remote = true;
int data_bot[8] = {0, 0, 0, 0, 0, 0, 0, 0};
int data_top[8] = {180, 180, 180, 180, 180, 180, 180, 0};
int flag = 0;
int param_pos[3] = {0, 13, 18};
const int LIFTS_COUNT = 6;
ServoSmooth LIFTS[LIFTS_COUNT];

void setup()
{
  digitalWrite(15, HIGH);
  digitalWrite(17, HIGH);
  digitalWrite(19, HIGH);

  for (int i = 0; i < LIFTS_COUNT; i++)
  {
    LIFTS[i].attach(LIFT_PINS[i]);
    pinMode(LIFT_PINS[i], OUTPUT);
    pinMode(LIFT_LIMIT_PINS[0][i], INPUT_PULLUP); // bottop stop switch
    pinMode(LIFT_LIMIT_PINS[1][i], INPUT_PULLUP); // top stop switch
  }

  Serial1.begin(115200);  // BRIDGE
  Serial2.begin(115200);  // SPREADER
  Serial3.begin(115200);  // MiniCranes
  Reciever.begin(115200); // TRANSMITTER

  Serial.begin(115200);

  pinMode(LIFTING_LED_PIN, OUTPUT);
  pinMode(MANUAL_LIFT_PIN, INPUT_PULLUP);

  oled.init();
  oled.clear();
  oled.print(
      F(
          " Servo1  90  180/r/n"
          " Servo2  90  180/r/n"
          " Servo3  90  180/r/n"
          " Servo4  90  180/r/n"
          " Servo5  90  180/r/n"
          " Servo6  90  180/r/n"
          " Srv1-6  90  180/r/n"
          " Remote/r/n"));

  oled.setCursor(0, pointer);
  oled.print('>');

  oled.update();
  oled.setPower(OLED_DISPLAY_OFF);
}

void loop()
{
  encoderServo.tick();
  if (digitalRead(MANUAL_LIFT_PIN) == LOW)
  {
    oled.setPower(OLED_DISPLAY_ON);
    if (encoderServo.turn())
    {
      switch (flag)
      {
      case 1:
        if (encoderServo.right())
        {
          data_bot[pointer]++;
          data_bot[pointer] = constrain(data_bot[pointer], 0, 90);
        }
        if (encoderServo.left())
        {
          data_bot[pointer]--;
          data_bot[pointer] = constrain(data_bot[pointer], 0, 90);
        }
        break;
      case 2:
        if (encoderServo.right())
        {
          data_top[pointer]++;
          data_top[pointer] = constrain(data_top[pointer], 90, 180);
        }
        if (encoderServo.left())
        {
          data_top[pointer]--;
          data_top[pointer] = constrain(data_top[pointer], 90, 180);
        }
        break;
      default:
        if (encoderServo.right())
          pointer++;
        if (encoderServo.left())
          pointer--;

        if (pointer > 7)
          pointer = 0;
        if (pointer < 0)
          pointer = 7;
        break;
      }

      for (int i = 0; i < 8; i++)
      {
        oled.setCursor(param_pos[0] * 6, i);
        oled.print(' ');

        oled.setCursor(param_pos[1] * 6, i);
        oled.print(' ');

        oled.setCursor(param_pos[2] * 6, i);
        oled.print(' ');
      }

      if (pointer < 7)
      {
        oled.setCursor((param_pos[1] + 1) * 6, pointer);
        oled.print(data_bot[pointer]);
        if (data_bot[pointer] < 10)
        {
          oled.setCursor((param_pos[1] + 2) * 6, pointer);
          oled.print(' ');
        }

        oled.setCursor((param_pos[2] + 1) * 6, pointer);
        oled.print(data_top[pointer]);
      }

      oled.setCursor(param_pos[flag] * 6, pointer);
      oled.print('>');
      oled.update();
      Serial.println(pointer);
    }

    if (encoderServo.click())
    {
      if (pointer == 7)
      {
        remote = !remote;
        oled.setCursor(6 * 2, 7);
        oled.print(remote ? "Remote" : "Manual");
        oled.update();
      }
      else // if(pointer > 0)
      {
        // if(flag == 2)
        // flag = 0;
        // else
        flag++;

        oled.setCursor(param_pos[flag == 0 ? 2 : flag - 1] * 6, pointer);
        oled.print(' ');
        oled.setCursor(param_pos[flag] * 6, pointer);
        oled.print('>');
        oled.update();
      }
    }

    if (millis() - tmr_oled > 50)
    {
      tmr_oled = millis();
      if (pointer < 7)
      {
        int val = map(analogRead(servoManualGimbalPin), 0, 1023, 0, 180);
        if (pointer == 6)
        {
          for (int i = 0; i < LIFTS_COUNT; i++)
          {
            LIFTS[i].write((val > 90 && digitalRead(LIFT_LIMIT_PINS[0][i]) == LOW) || (val < 90 && digitalRead(LIFT_LIMIT_PINS[1][i]) == LOW) ? val / 90 * (val > 90 ? data_top[i] : data_bot[i]) : 90);
          }
        }
        else
        {
          LIFTS[pointer].write(((val > 90 && digitalRead(LIFT_LIMIT_PINS[0][pointer]) == LOW) || (val < 90 && digitalRead(LIFT_LIMIT_PINS[1][pointer]) == LOW) ? val : 90));

          Serial.print("flag = ");
          Serial.print(flag);
          Serial.print("    pointer = ");
          Serial.print(pointer);
          Serial.print("    up = ");
          Serial.print(digitalRead(LIFT_LIMIT_PINS[0][pointer]));
          Serial.print("    bot = ");
          Serial.println(digitalRead(LIFT_LIMIT_PINS[1][pointer]));
        }
      }
    }
  }
  else
  {
    oled.setPower(OLED_DISPLAY_OFF);
  }

  if (Serial_Rx.available())
  {
    GParser data(Serial_Rx.buf);
    int ints[data.amount()];
    int n = data.parseInts(ints);

    switch (ints[0])
    {
    case 0:                         // BRIDGE
      Serial1.write(Serial_Rx.buf); // redirect to BRIDGE
      break;
    case 1:                         // SPREADER
      Serial2.write(Serial_Rx.buf); // redirect to SPREADER
      break;
    case 2:                         // MiniCranes
      Serial3.write(Serial_Rx.buf); // redirect to MiniCranes
      break;
    case 3: // Lifting
      int val = ints[2];
      int servoNum = ints[1];

      // if(millis() - tmr > 50)
      {
        // tmr = millis();

        if (servoNum == 6)
        {
          for (int i = 0; i < LIFTS_COUNT; i++)
          {
            LIFTS[i].write((val > 90 && digitalRead(LIFT_LIMIT_PINS[0][i]) == LOW) || (val < 90 && digitalRead(LIFT_LIMIT_PINS[1][i]) == LOW) ? val / 90 * (val > 90 ? data_top[i] : data_bot[i]) : 90);
          }
        }
        else
        {
          LIFTS[pointer].write(((val > 90 && digitalRead(LIFT_LIMIT_PINS[0][servoNum]) == LOW) || (val < 90 && digitalRead(LIFT_LIMIT_PINS[1][servoNum]) == LOW) ? val : 90));
        }
        break;
      }
    }
  }
}