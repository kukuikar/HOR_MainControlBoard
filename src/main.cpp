#include <SoftwareSerial.h>
#include <GParser.h>
#include <AsyncStream.h>
#include <ServoSmooth.h>
#include <GyverOLED.h>
#include <EncButton.h>
#include <Arduino.h>
#include <RF24.h>

//////////////////////////////////////////////////
///////////        DATA PACKAGE      /////////////
//////////////////////////////////////////////////
struct Data_Package
{
  byte Bridge_Enabled;
  byte Bridge_Drive;
  byte Bridge_Trolley;
  byte Bridge_Winch;

  byte Spreader_Enabled;
  byte Spreader_Rotate;
  byte Spreader_Telescopes;
  byte Spreader_Twistlocks;

  byte Mini_Enabled;
  byte Mini_Mode;
  byte Mini_ActiveCrane;
  byte Mini_Rotate;
  byte Mini_Winch;
  byte Mini_Arm;

  byte Lift_Enabled;
  byte Lift_Drive;
  byte Lift_Servo;
};

Data_Package data;

ServoSmooth s0;
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

///////////////////
/// PINS DEFINE ///
///////////////////
int LIFT_PINS[6] = {40, 41, 42, 43, 44, 45};
int LIFT_LIMIT_PINS[2][6] =
{
  {28, 30, 32, 34, 36, 38}, /*up limit*/
  {29, 31, 33, 35, 37, 39}, /*bottom limit*/
};


/////////////////////
/// COMMUNICATION ///
/////////////////////
#define TRANS_CE_PIN 9
#define TRANS_CSN_PIN 10

RF24 radio(TRANS_CE_PIN, TRANS_CSN_PIN);   // nRF24L01 (CE, CSN)
const byte address[6] = "00001";

unsigned long lastReceiveTime = 0;
unsigned long currentTime = 0;

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
int limit_value_bot[8] = {0, 0, 0, 0, 0, 0, 0, 0};
int limit_value_top[8] = {180, 180, 180, 180, 180, 180, 180, 0};
int flag = 0;
int param_pos[3] = {0, 9, 13};
const int LIFTS_COUNT = 6;
ServoSmooth LIFTS[LIFTS_COUNT];

void setup()
{
  //fix garbage in serials
  digitalWrite(15, HIGH);
  digitalWrite(17, HIGH);
  digitalWrite(19, HIGH);

  for (int i = 0; i < LIFTS_COUNT; i++)
  {
    //LIFTS[i].attach(LIFT_PINS[i]);
    pinMode(LIFT_PINS[i], OUTPUT);
    pinMode(LIFT_LIMIT_PINS[0][i], INPUT_PULLUP); // bottop stop switch
    pinMode(LIFT_LIMIT_PINS[1][i], INPUT_PULLUP); // top stop switch
  }

  s0.attach(LIFT_PINS[0]);

  Serial1.begin(115200);  // BRIDGE
  Serial2.begin(115200);  // SPREADER
  Serial3.begin(115200);  // MiniCranes

  Serial.begin(38400);

  radio.begin();
  radio.openReadingPipe(0, address);
  radio.setAutoAck(false);
  radio.setDataRate(RF24_250KBPS);
  radio.setPALevel(RF24_PA_LOW);
  radio.startListening(); //  Set the module as receiver
  resetData();
}

void loop()
{  
////////////////////////////////////
/// что-то получено в приемник /////
////////////////////////////////////

if (radio.available())
{
  radio.read(&data, sizeof(Data_Package)); // Read the whole data and store it into the 'data' structure
  lastReceiveTime = millis(); // At this moment we have received the data
}

currentTime = millis();

 if ( currentTime - lastReceiveTime > 1000 )
 {
  // If current time is more then 1 second since we have recived the last data, that means we have lost connection
    resetData(); // If connection is lost, reset the data. It prevents unwanted behavior, for example if a drone has a throttle up and we lose connection, it can keep flying unless we reset the values
 }
 
 if(data.Bridge_Enabled == 0)
 {
  Serial1.write('0,');
  Serial1.write(map(data.Bridge_Drive, 0, 255, -255, 255));
  Serial1.write(',');
  Serial1.write(map(data.Bridge_Trolley, 0, 255, -255, 255));
  Serial1.write(',');
  Serial1.write(map(data.Bridge_Winch, 0, 255, -255, 255));
  Serial1.write(';');
 }
 else if(data.Spreader_Enabled == 0)
 {
  Serial2.write('1,');
  Serial2.write(map(data.Spreader_Rotate, 0, 255, -255, 255));
  Serial2.write(',');
  Serial2.write(map(data.Spreader_Telescopes, 0, 255, -255, 255));
  Serial2.write(',');
  Serial2.write(data.Spreader_Twistlocks);
  Serial2.write(';');
 }
 else if(data.Mini_Enabled == 0)
 {
  Serial3.write('2,');
  Serial3.write(data.Mini_Mode);
  Serial3.write(',');
  Serial3.write(data.Mini_ActiveCrane);
  Serial3.write(',');
  Serial3.write(map(data.Mini_Rotate, 0, 255, -255, 255));
  Serial3.write(',');
  Serial3.write(map(data.Mini_Winch, 0, 255, -255, 255));
  Serial3.write(',');
  Serial3.write(map(data.Mini_Arm, 0, 255, -255, 255));
  Serial3.write(';');
 }
 else if(data.Lift_Enabled == 0)
 {
  int s = data.Lift_Servo;
  int val = data.Lift_Drive;
  s0.write((val > 90 && digitalRead(LIFT_LIMIT_PINS[0][0]) == LOW) || (val < 90 && digitalRead(LIFT_LIMIT_PINS[1][0]) == LOW) ? val : 90);
  //LIFTS[i].write((val > 90 && digitalRead(LIFT_LIMIT_PINS[0][i]) == LOW) || (val < 90 && digitalRead(LIFT_LIMIT_PINS[1][i]) == LOW) ? val / 90 * (val > 90 ? limit_value_top[i] : limit_value_bot[i]) : 90);    
 }
 else
 {
  resetData();
 }
}

void resetData() {
  // Reset the values when there is no radio connection - Set initial default values
  data.Bridge_Enabled = 1;
  data.Spreader_Enabled = 1;
  data.Mini_Enabled = 1;
  data.Lift_Enabled = 1;
  data.Lift_Drive = 90;

  for (size_t i = 0; i < LIFTS_COUNT; i++)
  {
    LIFTS[i].write(90);
  }  
}