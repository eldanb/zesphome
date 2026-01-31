#pragma once

#include <cstdint>

namespace esphome
{
  namespace rfm69
  {
    class Rfm69;
  }

  namespace rfm69_pellini_cover
  {

    class PelliniRemote
    {
    public:
      PelliniRemote(esphome::rfm69::Rfm69 *transmitter, uint8_t address);
      void init();

      enum PelliniRemoteCommand
      {
        PelliniRemoteCommandDown = 0xda,
        PelliniRemoteCommandUp = 0xf2,
      };

      void send_command(PelliniRemoteCommand cmd, int count, int last_delay = 20);
      void send_command_for_duration(PelliniRemoteCommand cmd, int duration);

      int getAddress();

    private:
      void compose_frame(uint8_t ctrl, uint8_t digitalOut[6]);
      void frame_to_ook_packet(uint8_t frame[6], uint8_t packet[], int *packet_len);

      uint8_t _address;
      esphome::rfm69::Rfm69 *_transmitter;
    };

  }
}
