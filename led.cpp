#include <Preferences.h>
#include "led.hpp"

//Initalize with the Default State as an Orange
LED::LED() : red{0xFA}, green{0x6B}, blue{0x05}, whiteTemprature{0x00}, brightness{0x00}
{
}

LED::LED(uint8_t redPart, uint8_t greenPart, uint8_t bluePart, uint8_t temprature, uint8_t intensity) : red{redPart}, green{greenPart}, blue{bluePart}, whiteTemprature{temprature}, brightness{intensity}
{
}

bool LED::setColor(uint8_t *RGB_LED)
{
  red =RGB_LED[0];
  green = RGB_LED[1];
  blue = RGB_LED[2];
  return 0;
}

bool LED::setColor(uint8_t redPart, uint8_t greenPart, uint8_t bluePart)
{
  red = redPart;
  blue = bluePart;
  green = greenPart;

  return 0;
}

bool LED::setTemprature(uint8_t temprature)
{
  whiteTemprature = temprature;

  return 0;
}

bool LED::setBrightness(uint8_t intensity)
{
  brightness = intensity;

  return 0;
}

uint8_t LED::getRed()
{
  return red;
}

uint8_t LED::getBlue()
{
  return blue;
}

uint8_t LED::getGreen()
{
  return green;
}

uint8_t LED::getTemprature()
{
  return whiteTemprature;
}

uint8_t LED::getBrightness()
{
  return brightness;
}

/*
  Save every State of the LED's to the given Non Volatile Memory (nvm)
  The namespace of the Prefrence must already be open
*/
void LED::saveState(Preferences* nvm)
{
  uint8_t buffer[5];
  buffer[0] = nvm->putUChar("Red",red);
  buffer[1] = nvm->putUChar("Green",green);
  buffer[2] = nvm->putUChar("Blue",blue);
  buffer[3] = nvm->putUChar("Brightness",brightness);
  buffer[4] = nvm->putUChar("Temprature",whiteTemprature);

  for (int i = 0; i<5; i++) 
  {
    Serial.write(buffer[i]);
  }
}


/*
  Load every State of the LED's from the given Non Volatile Memory (nvm) 
  The namespace of the Prefrence must already be open
*/
void LED::loadState(Preferences* nvm)
{
  red = nvm->getUChar("Red");
  green = nvm->getUChar("Green");
  blue = nvm->getUChar("Blue");
  brightness = nvm->getUChar("Brightness");
  whiteTemprature = nvm->getUChar("Temprature");
}