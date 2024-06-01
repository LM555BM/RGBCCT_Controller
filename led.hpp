#pragma once

#include <Preferences.h>
#include <stdint.h>

enum color_t
{
  RED,
  GREEN,
  BLUE,
  TEMPRATURE,
  BRIGHTNESS
};

class LED
{
private:
  /*
  color = [red, green, blue, Temprature, Brightness]
  */
  std::array< uint8_t, 5 > color;

public:
  LED(uint8_t redPart = 0xFF, uint8_t greenPart = 0x64, uint8_t bluePart = 0x00, uint8_t temprature = 0x00, uint8_t intensity = 0x00);

  /*
    RGB = [red,green,blue]
    */
  void setColor(std::array< uint8_t, 5 >* colorData);
  bool setRGB(uint8_t* RGB);
  bool setRGB(uint8_t redPart, uint8_t greenPart, uint8_t bluePart);
  bool setTemprature(uint8_t temprature);
  bool setBrightness(uint8_t intensity);

  std::array< uint8_t, 5 >& getColor();
};
