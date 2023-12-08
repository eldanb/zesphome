#pragma once

#include "esphome/core/preferences.h"
#include <Arduino.h>

namespace esphome
{
  namespace rfm69
  {
    class Rfm69;
  }

  namespace rfm69_somfy_cover
  {
    class SomfyRemote
    {
    public:
      SomfyRemote(esphome::rfm69::Rfm69 *transmitter, long address);
      void init();

      enum SomfyRemoteCommand
      {
        SomfyRemoteCommandMy = 0x01,
        SomfyRemoteCommandUp = 0x02,
        SomfyRemoteCommandDown = 0x04,
        SomfyRemoteCommandProg = 0x08
      };

      void send_command(SomfyRemoteCommand cmd, int count);

      long getAddress();
      long getRollingCode();

    private:
      void load_rolling_code();
      void increment_rolling_code();
      void compose_frame(byte ctrl, byte digitalOut[7]);
      void frame_to_ook_packet(byte frame[7], byte packet[], int *packet_len);

      rfm69::Rfm69 *_transmitter;
      long _address;
      long _rolling_code;
      ESPPreferenceObject _pref;
    };
  }
}
