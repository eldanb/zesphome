#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"
#include "esphome/core/application.h"

#include <Adafruit_NeoPixel.h>

namespace esphome
{
  namespace fastled_status_led
  {
    static const char *const TAG = "fastled_status_led";

    class FastLEDStatusLED : public Component
    {
    public:
      FastLEDStatusLED(int pinNumber)
          : _pixel(1, pinNumber, NEO_GRB + NEO_KHZ400), _nextUpdateMillis(0), _lastStatus(-1)
      {
      }

      void setup()
      {
        _pixel.begin();
      }

      void loop()
      {
        int curMillis = millis();
        if (curMillis < _nextUpdateMillis)
        {
          return;
        }

        _nextUpdateMillis = curMillis + 500;

        if ((App.get_app_state() & STATUS_LED_ERROR) != 0u)
        {
          showState(STATUS_LED_ERROR);
        }
        else if ((App.get_app_state() & STATUS_LED_WARNING) != 0u)
        {
          showState(STATUS_LED_WARNING);
        }
        else
        {
          showState(0);
        }
      }

    private:
      void showState(int state)
      {
        if (_lastStatus == state)
        {
          return;
        }

        _lastStatus = state;

        ESP_LOGD(TAG, "Setting state %d", state);
        if (state == STATUS_LED_ERROR)
        {
          _pixel.setPixelColor(0, 128, 0, 0);
        }
        else if (state == STATUS_LED_WARNING)
        {
          _pixel.setPixelColor(0, 128, 128, 0);
        }
        else
        {
          _pixel.setPixelColor(0, 0, 64, 0);
        }

        _pixel.show();
      }

    private:
      Adafruit_NeoPixel _pixel;
      int _nextUpdateMillis;
      int _lastStatus;
    };
  }
}
