#include "led.hpp"
#include <Preferences.h>

//Initalize with the Default State as an Orange
LED::LED(uint8_t redPart, uint8_t greenPart, uint8_t bluePart, uint8_t temprature, uint8_t intensity)
{
  color[RED] = redPart;
  color[GREEN] = greenPart;
  color[BLUE] = bluePart;
  color[TEMPRATURE] = temprature;
  color[BRIGHTNESS] = intensity;
}

void LED::setColor(std::array< uint8_t, 5 >* colorData)
{
  color = *colorData;
}

bool LED::setRGB(uint8_t* RGB_LED)
{
  color[RED] = RGB_LED[0];
  color[GREEN] = RGB_LED[1];
  color[BLUE] = RGB_LED[2];
  return 0;
}

bool LED::setRGB(uint8_t redPart, uint8_t greenPart, uint8_t bluePart)
{
  color[RED] = redPart;
  color[GREEN] = greenPart;
  color[BLUE] = bluePart;

  return 0;
}

bool LED::setTemprature(uint8_t temprature)
{
  color[TEMPRATURE] = temprature;

  return 0;
}

bool LED::setBrightness(uint8_t intensity)
{
  color[BRIGHTNESS] = intensity;

  return 0;
}

std::array< uint8_t, 5 >& LED::getColor()
{
  return color;
}