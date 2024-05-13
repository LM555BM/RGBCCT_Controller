#pragma once

#include <stdint.h>
#include <Preferences.h>

class LED
{
  private:
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t whiteTemprature;
    uint8_t brightness;
  public:
    LED();
    LED(uint8_t redPart, uint8_t greenPart, uint8_t bluePart, uint8_t temprature, uint8_t intensity);

    /*
    RGB = [red,green,blue]
    */
    bool setColor(uint8_t *RGB);
    bool setColor(uint8_t redPart, uint8_t greenPart, uint8_t bluePart);
    bool setTemprature(uint8_t temprature);
    bool setBrightness(uint8_t intensity);

    uint8_t getRed();
    uint8_t getBlue();
    uint8_t getGreen();
    uint8_t getTemprature();
    uint8_t getBrightness();

    void saveState(Preferences * nvm);
    void loadState(Preferences* nvm);
};
