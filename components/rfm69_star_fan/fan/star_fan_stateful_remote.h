#pragma once

#include <cstdint>

namespace esphome
{
  namespace rfm69
  {
    class Rfm69;
  }

  namespace rfm69_star_fan
  {
    using rfm69::Rfm69;

    class StarFanStatefulRemote
    {
    public:
      StarFanStatefulRemote(Rfm69 *transmitter, uint8_t address);
      void init();

      void reset();

      void setPower(bool power);
      void setSpeed(int speed);
      void setDirection(bool ccw);
      void setLight(bool lightOn);

      int getSpeed() { return _speed; }
      bool getPower() { return _fanOn; }
      bool getDirection() { return _dirCcw; }
      bool getLight() { return _lightOn; }

      // 6 bit
      uint8_t getAddress();

    private:
      // 7bit
      enum StarFanRemoteCommand
      {
        StarFanRemoteCommandOff = 0x04,
        StarFanRemoteCommandLight = 0x02,
        StarFanRemoteCommandDir = 0x08,
        StarFanRemoteCommandSpd1 = 0x10,
        StarFanRemoteCommandSpd2 = 0x14,
        StarFanRemoteCommandSpd3 = 0x20,
        StarFanRemoteCommandSpd4 = 0x30,
        StarFanRemoteCommandSpd5 = 0x44,
        StarFanRemoteCommandSpd6 = 0x40,
      };

      void command_to_ook_packet(StarFanRemoteCommand cmd, uint8_t packet[], int *packet_len);
      void send_command(StarFanRemoteCommand cmd);

      bool _dirCcw;
      bool _lightOn;
      bool _fanOn;
      int _speed;

      uint8_t _address;
      Rfm69 *_transmitter;
    };

  }
}
