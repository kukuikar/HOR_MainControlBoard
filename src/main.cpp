//#include <SoftwareSerial.h>
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
  byte Bridge_Enabled = 1;
  byte Bridge_Drive = 0;
  byte Bridge_Trolley = 0;
  byte Bridge_Winch = 0;

  byte Spreader_Enabled = 1;
  byte Spreader_Rotate = 0;
  byte Spreader_Telescopes = 0;
  byte Spreader_Twistlocks = 90;

  byte Mini_Enabled = 1;
  byte Mini_Mode = 0;
  byte Mini_ActiveCrane = 0;
  byte Mini_Rotate = 0;
  byte Mini_Winch = 0;
  byte Mini_Arm = 0;

  byte Lift_Enabled = 1;
  byte Lift_Drive = 90;
  byte Lift_Servo = 6;
};
Data_Package data;

/////////////////////////////////////////////
///////// SERVO FOR TESTING  ////////////////
///////// REMOVE AFTER DEBUG ////////////////
/////////////////////////////////////////////

ServoSmooth LIFTS[6]; //инициализировал 6 серв

/////////////////////////////////////////////

/////////////////////////////////////////////
/// PINS DEFINE                           ///
/////////////////////////////////////////////
int LIFT_PINS[6] = {40, 41, 42, 43, 44, 45};
int LIFT_LIMIT_PINS[2][6] =
    {
        {28, 30, 32, 34, 36, 38}, /*up limit*/
        {29, 31, 33, 35, 37, 39}, /*bottom limit*/
};

/////////////////////////////////////////////
/// COMMUNICATION                         ///
/////////////////////////////////////////////
#define TRANS_CE_PIN 9
#define TRANS_CSN_PIN 10

RF24 radio(TRANS_CE_PIN, TRANS_CSN_PIN); // nRF24L01 (CE, CSN)
const byte address[6] = "00001";

unsigned long lastReceiveTime = 0;
unsigned long currentTime = 0;

/////////////////////////////////////////////
/// TIMERS                                ///
/////////////////////////////////////////////
uint32_t tmr = millis();
uint32_t tmr_oled = millis();
int tmr_oled_inerval = 200;

/////////////////////////////////////////////
/// VARIABLES                             ///
/////////////////////////////////////////////
int LIMIT_VALUE_BOT[6] = {0, 0, 0, 0, 0, 0};
int LIMIT_VALUE_TOP[6] = {180, 180, 180, 180, 180, 180};
const int LIFTS_COUNT = 6;


void setup()
{
  // fix garbage in serials
  digitalWrite(15, HIGH);
  digitalWrite(17, HIGH);
  digitalWrite(19, HIGH);

  for (int i = 0; i < LIFTS_COUNT; i++)
  {
    LIFTS[i].attach(LIFT_PINS[i]);
    pinMode(LIFT_PINS[i], OUTPUT);
    pinMode(LIFT_LIMIT_PINS[0][i], INPUT_PULLUP); // bottop stop switch pullup
    pinMode(LIFT_LIMIT_PINS[1][i], INPUT_PULLUP); // top stop switch pullup
  }

  /////////////////////////////////////////////
  ///////// SERVO FOR TESTING  ////////////////
  ///////// REMOVE AFTER DEBUG ////////////////
  /////////////////////////////////////////////
  

  /////////////////////////////////////////////

  /*Serial1.begin(115200); // BRIDGE
  Serial2.begin(115200); // SPREADER
  Serial3.begin(115200); // MINICRANES
*/
  Serial.begin(38400);

  // begin rf24 session
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
  /// something recieved in rf24 /////
  ////////////////////////////////////
  if (radio.available())
  {
    radio.read(&data, sizeof(Data_Package)); // Read the whole data and store it into the 'data' structure
    lastReceiveTime = millis();              // At this moment we have received the data

    // Добавил, чтобы проверить нажатие тумблеров
    Serial.println(data.Lift_Drive);
    Serial.print("Bridge = ");
    Serial.println(data.Bridge_Enabled);
    Serial.print("Spreader = ");
    Serial.println(data.Spreader_Enabled);
    Serial.print("Mini = ");
    Serial.println(data.Mini_Enabled);
    Serial.print("Lift = ");
    Serial.println(data.Lift_Enabled);
    //////////////////////////////////////////////

  }
  
  
  currentTime = millis();

  if (currentTime - lastReceiveTime > 1000)
  {
    // If current time is more then 1 second since we have recived the last data, that means we have lost connection
    resetData(); // If connection is lost, reset the data. It prevents unwanted behavior
  }

  
  /*if (data.Bridge_Enabled == 0 && data.Spreader_Enabled != 0 && data.Mini_Enabled != 0 && data.Lift_Enabled != 0)
  {//BRIDGE ENABLED
    Serial1.write('0,');
    Serial1.write(map(data.Bridge_Drive, 0, 255, -255, 255));
    Serial1.write(',');
    Serial1.write(map(data.Bridge_Trolley, 0, 255, -255, 255));
    Serial1.write(',');
    Serial1.write(map(data.Bridge_Winch, 0, 255, -255, 255));
    Serial1.write(';');
  }
  else if (data.Bridge_Enabled != 0 && data.Spreader_Enabled == 0 && data.Mini_Enabled != 0 && data.Lift_Enabled != 0)
  {//SPREADER ENABLED
    Serial2.write('1,');
    Serial2.write(map(data.Spreader_Rotate, 0, 255, -255, 255));
    Serial2.write(',');
    Serial2.write(map(data.Spreader_Telescopes, 0, 255, -255, 255));
    Serial2.write(',');
    Serial2.write(data.Spreader_Twistlocks);
    Serial2.write(';');
  }
  else if (data.Bridge_Enabled != 0 && data.Spreader_Enabled != 0 && data.Mini_Enabled == 0 && data.Lift_Enabled != 0)
  {//MINI CRANES ENABLED
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
  }*/
  else if (data.Bridge_Enabled != 0 && data.Spreader_Enabled != 0 && data.Mini_Enabled != 0 && data.Lift_Enabled == 0)
  {//LIFTS ENABLED
    int s = data.Lift_Servo;
    int val = data.Lift_Drive;
    /////////////////////////////////////////////
    ///////// SERVO FOR TESTING  ////////////////
    ///////// REMOVE AFTER DEBUG ////////////////
    /////////////////////////////////////////////


//------------------Управление сервами-------------------


    if (val>=80 && val <=120){   // добавил, так как у "большого" джойстика прыгали значения и при каждом новом подключение среднее значение сдвигалось
      stop();
    }

    else{
      for (int i=0;i<LIFTS_COUNT;i++){
        LIFTS[i].write(((val > 90 && digitalRead(LIFT_LIMIT_PINS[0][i]) == LOW) || (val < 90 && digitalRead(LIFT_LIMIT_PINS[1][i]) == LOW)) ? val : 90);
      }
    }

//-------------------------------------------------------    



    //LIFTS[i].write((val > 90 && digitalRead(LIFT_LIMIT_PINS[0][i]) == LOW) || (val < 90 && digitalRead(LIFT_LIMIT_PINS[1][i]) == LOW) ? val / 90 * (val > 90 ? LIMIT_VALUE_TOP[i] : LIMIT_VALUE_BOT[i]) : 90);
  
  }
  else
  {
    /////////////////////////////////////////////
    ///////// SERVO FOR TESTING  ////////////////
    ///////// REMOVE AFTER DEBUG ////////////////
    /////////////////////////////////////////////
    stop();
    /////////////////////////////////////////////
    resetData();
  }
}

void resetData()
{
  // Reset the values when there is no radio connection - Set initial default values
  data.Bridge_Enabled = 1;
  data.Bridge_Drive   = 0;
  data.Bridge_Trolley = 0;
  data.Bridge_Winch   = 0;

  data.Spreader_Enabled     = 1;
  data.Spreader_Rotate      = 0;
  data.Spreader_Telescopes  = 0;
  data.Spreader_Twistlocks  = 90;

  data.Mini_Enabled     = 1;
  data.Mini_Mode        = 0;
  data.Mini_ActiveCrane = 0;
  data.Mini_Rotate      = 0;
  data.Mini_Winch       = 0;
  data.Mini_Arm         = 0;

  data.Lift_Enabled = 1;
  data.Lift_Drive   = 90;
  data.Lift_Servo   = 6;

  for (int i = 0; i < LIFTS_COUNT; i++)
  {
    LIFTS[i].write(90);
  }
}


void stop(){     //Добавил функцию остановки всех серв
   for (int i=0;i<LIFTS_COUNT;i++){
      LIFTS[i].write(90);
    }
}