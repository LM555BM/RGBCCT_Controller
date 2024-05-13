#include <WiFi.h>
#include <Preferences.h>
#include "driver/touch_pad.h"
#include "esp_now_connect.hpp"
#include "led.hpp"

//Enums
typedef enum
{
  Start = 0xC9,
  Datenart,
  Framesize,
  Datenpaket = 0xDA,
  Kommandopaket = 0xC0,
  Helligkeitsbefehl = 0x0A,
  Tempraturbefehl = 0xFF,
  Speichern = 0x01,
  Ende = 0x36
}state_t;

//IO-Pinout Config
const uint8_t RedLED = 33;
const uint8_t GreenLED = 32;
const uint8_t BlueLED = 27;
const uint8_t WarmLED = 26;
const uint8_t ColdLED = 25;
const uint8_t debugLED = 2;

const touch_pad_t firstPad = TOUCH_PAD_NUM8;
const touch_pad_t secondPad = TOUCH_PAD_NUM9;

//Gloable Variable and Instances
bool longPressDetected = false;
bool shortPressDetected = false;
uint16_t touchValue = 0;

LED myLight[10];
bool updateAvailable = false;
static uint16_t state = Start;

Preferences eeprom;

//ISR
void touch_pad_pressed_isr(void* arg)
{
  touch_pad_read(firstPad, &touchValue);

  Serial.println("Touch ISR");

  static hw_timer_t* usedTimer = static_cast<hw_timer_t*>(arg);
  static uint32_t currentPad = 0xFFFF;
  uint32_t touchStatus = touch_pad_get_status();
  touch_pad_clear_status();

  if(touchStatus & BIT8 & currentPad)
  {
    if(currentPad == 0xFFFF)
    {
      currentPad = BIT8;

      timerRestart(usedTimer);
      timerAlarmEnable(usedTimer);
      timerStart(usedTimer);

      touch_pad_set_trigger_mode(TOUCH_TRIGGER_ABOVE);
    }

    else
    {
      timerStop(usedTimer);
      timerAlarmDisable(usedTimer);
      if(timerReadMilis(usedTimer) > 50 && timerReadMilis(usedTimer) < 3000)
      {
        shortPressDetected = true;
      }

      touch_pad_set_trigger_mode(TOUCH_TRIGGER_BELOW);
      currentPad = 0xFFFF;
      
    }
  }
}

void longPress_isr(void)
{
  longPressDetected = true;
}

void serialEvent()
{
  static uint16_t length;
  static uint8_t artworkCounter = 0;
  static uint8_t neededBytes = 1;
  uint8_t buffer[3];

  while(Serial.available() >= neededBytes)
  {
    switch(state & 0x00FF)
    {
      case Start:
        if(Serial.read() == Start)
        {
          state = Datenart;
        }
        break;
      case Datenart:
        state = Serial.read() << 8 | Framesize;
        neededBytes = 2;
        break;

      case Framesize:
        Serial.readBytes(buffer,neededBytes);
        length = buffer[0] << 8 | buffer[1];

        state = state >> 8;
        if(state == Datenpaket)
        {
          neededBytes = 3;
        }
        else if(state & 0xFF00 == Kommandopaket)
        {
          neededBytes = 1;
        }
        break;

      case Datenpaket:
        Serial.readBytes(buffer,3);
        myLight[artworkCounter++].setColor(buffer);
        length = length - 3;

        if(length == 0)
        {
          neededBytes = 1;
          state = Ende;
        }
        break;

      case Kommandopaket:
        state = Serial.read();
        break;

      case Helligkeitsbefehl:
        buffer[0] = Serial.read();
        myLight[artworkCounter++].setBrightness(buffer[0]);

        if(--length == 0)
        {
          neededBytes = 1;
          state = Ende;
        }
        break;

      case Tempraturbefehl:
        buffer[0] = Serial.read();
        myLight[artworkCounter++].setTemprature(buffer[0]);

        if(--length == 0)
        {
          neededBytes = 1;
          state = Ende;
        }
        break;

      case Ende:
        if(Serial.read() == Ende)
        {
          artworkCounter = 0;
          neededBytes = 1;
          state = Start;
          updateAvailable = true;
        }
        break;
    }
  }
}

//Functions
void updateLED()
{
  ledcWrite(0, myLight[0].getRed());
  ledcWrite(1, myLight[0].getGreen());
  ledcWrite(2, myLight[0].getBlue());
  ledcWrite(3, (myLight[0].getTemprature() * myLight[0].getBrightness()) / 255);
  ledcWrite(4, ((255 - myLight[0].getTemprature()) * myLight[0].getBrightness()) / 255);
}

void setup()
{
  //Setup the Pins which drives the MOSFET to use the LEDC Controller on the ESP32
  ledcAttachPin(RedLED, 0);
  ledcAttachPin(GreenLED, 1);
  ledcAttachPin(BlueLED, 2);
  ledcAttachPin(WarmLED, 3);
  ledcAttachPin(ColdLED, 4);

  ledcSetup(0, 1000, 8);
  ledcSetup(1, 1000, 8);
  ledcSetup(2, 1000, 8);
  ledcSetup(3, 1000, 8);
  ledcSetup(4, 1000, 8);

  //Setup Touchpad with Timer for long Press detection
  hw_timer_t* timer1 = timerBegin(0,24000,true);
  timerStop(timer1);
  timerAttachInterrupt(timer1, longPress_isr, true);
  timerAlarmWrite(timer1,10000,false);

  touch_pad_init();
  esp_sleep_enable_touchpad_wakeup();
  touch_pad_set_fsm_mode(TOUCH_FSM_MODE_TIMER);
  touch_pad_config(firstPad, 200);
  touch_pad_set_trigger_mode(TOUCH_TRIGGER_BELOW);
  touch_pad_isr_register(touch_pad_pressed_isr,static_cast<void*>(timer1));
  touch_pad_intr_enable();
  
  //Restore the State of the LED's if available in the EEPROM, else create default
  eeprom.begin("LEDState",true);
  bool isInit = eeprom.isKey("init");

  if(!isInit)
  {
    eeprom.end();
    eeprom.begin("LEDState",false);
    myLight[0].saveState(&eeprom);
    eeprom.putBool("init",true);
  }
  else
  {
    myLight[0].loadState(&eeprom);
  }

  eeprom.end();
  updateLED();

  //Miscellaneous stuff
  Serial.begin(9600);

  while(!Serial){}

  WiFi.mode(WIFI_STA);

  pinMode(debugLED,OUTPUT);
}

void loop()
{
    if(updateAvailable)
  {
    updateLED();

    updateAvailable = false;
  }

  //Need's to be in the Loop as serialEvent won't be open, because their is no Data needed for saving
  if(state == Speichern)
  {
    //To Do let the other's know they need so save 
    eeprom.begin("LEDState",false);
    myLight[0].saveState(&eeprom);
    eeprom.end();
    
    state = Ende;
  }
}