#include <WiFi.h>
#include <Preferences.h>
#include "driver/touch_pad.h"
#include "esp_now_connect.hpp"
#include "led.hpp"

//Enums
enum serial_state_t
{
  START = 0xC9,
  DATENART,
  FRAMESIZE,
  DATENPAKET = 0xDA,
  KOMMANDOPAKET = 0xC0,
  HELLIGKEITSBEFEHL = 0x0A,
  TEMPRATURBEFEHL = 0xFF,
  SPEICHERN = 0x01,
  ENDE = 0x36
};

enum loop_state_t
{
  IDLE,
  SAVE_LED,
  UPDATE_LED,
  POWER_SAVING
};

//IO-Pinout Config
const uint8_t RedPin = 19;
const uint8_t GreenPin = 18;
const uint8_t BluePin = 5;
const uint8_t WarmPin = 2;
const uint8_t ColdPin = 4;

const uint8_t RedLED = 0;
const uint8_t GreenLED = 1;
const uint8_t BlueLED = 2;
const uint8_t WarmLED = 3;
const uint8_t ColdLED = 4;

const uint8_t debugLED = 2;

const touch_pad_t firstPad = TOUCH_PAD_NUM8;
const touch_pad_t secondPad = TOUCH_PAD_NUM9;

//Gloable Variable and Instances
bool longPressDetected = false;
bool shortPressDetected = false;
uint16_t touchValue = 0;

std::vector<LED> myLight;
static uint16_t serialState = START;
loop_state_t loopState = IDLE;

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
  Serial.write(serialState & 0x00FF);

  static uint16_t length;
  static uint8_t artworkCounter = 0;
  static uint8_t neededBytes = 1;
  uint8_t buffer[3];

  while(Serial.available() >= neededBytes)
  {
    switch(serialState & 0x00FF)
    {
      case START:
        if(Serial.read() == START)
        {
          serialState = DATENART;
        }
        break;
      case DATENART:
        serialState = Serial.read() << 8 | FRAMESIZE;
        neededBytes = 2;
        break;

      case FRAMESIZE:
        Serial.readBytes(buffer,neededBytes);
        length = buffer[0] << 8 | buffer[1];

        if(length / neededBytes > myLight.size())
        {
          myLight.reserve(length / neededBytes);
        }
        

        serialState = serialState >> 8;
        if(serialState == DATENPAKET)
        {
          neededBytes = 3;
        }
        else if(serialState & 0xFF00 == KOMMANDOPAKET)
        {
          neededBytes = 1;
        }
        break;

      case DATENPAKET:
        Serial.readBytes(buffer,3);
        try
        {
          myLight.at(artworkCounter++).setRGB(buffer);
        }
        catch(std::out_of_range const& exc)
        {
          myLight.push_back(LED(buffer[RED],buffer[GREEN],buffer[BLUE]));
        }
        
        length = length - 3;

        if(length == 0)
        {
          neededBytes = 1;
          serialState = ENDE;
        }
        break;

      case KOMMANDOPAKET:
        serialState = Serial.read();
        break;

      case HELLIGKEITSBEFEHL:
        //There is currently only one Brightness for everybody
        buffer[0] = Serial.read();
        for(auto& artwork : myLight)
        {
          artwork.setBrightness(buffer[0]);
        }

        // Use when seprate Brightnesses are available
        //myLight[artworkCounter++].setBrightness(buffer[0]);

        if(--length == 0)
        {
          neededBytes = 1;
          serialState = ENDE;
        }
        break;

      case TEMPRATURBEFEHL:
        buffer[0] = Serial.read();
        for(auto& artwork : myLight)
        {
          artwork.setTemprature(buffer[0]);
        }

        // Use when seprate Tempratures are available
        myLight[artworkCounter++].setTemprature(buffer[0]);

        if(--length == 0)
        {
          neededBytes = 1;
          serialState = ENDE;
        }
        break;

      case ENDE:
        if(Serial.read() == ENDE)
        {
          artworkCounter = 0;
          neededBytes = 1;
          serialState = START;
          loopState = UPDATE_LED;
        }
        break;
    }
  }
}

void setup()
{
  //Setup the Pins which drives the MOSFET to use the LEDC Controller on the ESP32
  ledcAttachPin(RedPin, 0);
  ledcAttachPin(GreenPin, 1);
  ledcAttachPin(BluePin, 2);
  ledcAttachPin(WarmPin, 3);
  ledcAttachPin(ColdPin, 4);

  ledcSetup(RedLED, 1000, 8);
  ledcSetup(GreenLED, 1000, 8);
  ledcSetup(BlueLED, 1000, 8);
  ledcSetup(WarmLED, 1000, 8);
  ledcSetup(ColdLED, 1000, 8);

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
  myLight.push_back(isInit ? LED(eeprom.getUChar("Red"), eeprom.getUChar("Green"), eeprom.getUChar("Blue"), eeprom.getUChar("Brightness"), eeprom.getUChar("Temprature")) : LED());
  eeprom.end();
  loopState = UPDATE_LED;

  //Miscellaneous stuff
  Serial.begin(9600);

  while(!Serial){}

  WiFi.mode(WIFI_STA);

  pinMode(debugLED,OUTPUT);
}

void loop()
{
  switch(loopState)
  {
    case IDLE:
      {
        
      }
      break;
    
    case POWER_SAVING:
    {
      break;
    }

    case SAVE_LED:
    {
      uint8_t error = 1;

      eeprom.begin("LEDState",false);

      std::array<uint8_t, 5>& saveColor = myLight[0].getColor();

      error &= eeprom.putUChar("Red",saveColor[RED]);
      error &= eeprom.putUChar("Green",saveColor[GREEN]);
      error &= eeprom.putUChar("Blue",saveColor[BLUE]);
      error &= eeprom.putUChar("Brightness",saveColor[BRIGHTNESS]);
      error &= eeprom.putUChar("Temprature",saveColor[TEMPRATURE]);

      eeprom.putBool("init",true);
      eeprom.end();

      if(error != 0)
      {
        Serial.println("An error has occured while saving to NVM");
      }
      //Inform others

      loopState = IDLE;
      break;
    }

    case UPDATE_LED:
    {
      std::array<uint8_t, 5>& newColor = myLight[0].getColor();

      ledcWrite(RedLED, newColor[RED]);
      ledcWrite(GreenLED, newColor[GREEN]);
      ledcWrite(BlueLED, newColor[BLUE]);
      ledcWrite(WarmLED, (newColor[TEMPRATURE] * newColor[BRIGHTNESS]) / 255);
      ledcWrite(ColdLED, ((255 - newColor[TEMPRATURE] * newColor[BRIGHTNESS]) / 255));

      //Inform others

      loopState = IDLE;
      break;
    }
  }

  //Need's to be in the Loop as serialEvent won't be open, because their is no Data needed for saving
  if(serialState == SPEICHERN)
  {
    loopState = SAVE_LED;
    serialState = ENDE;
  }
}