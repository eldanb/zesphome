#include "star_fan_stateful_remote.h"

#include "esphome/components/rfm69/rfm69.h"
#include <memory.h>

namespace esphome
{
  namespace rfm69
  {
    class Rfm69;
  }

  namespace rfm69_star_fan
  {
    using rfm69::Rfm69;

    StarFanStatefulRemote::StarFanStatefulRemote(Rfm69 *transmitter, uint8_t address)
        : _transmitter(transmitter), _address(address)
    {
      reset();
    }

    void StarFanStatefulRemote::init()
    {
    }

    void StarFanStatefulRemote::reset()
    {
      this->_dirCcw = false;
      this->_lightOn = false;
      this->_fanOn = false;
      this->_speed = 1;
    }

    void StarFanStatefulRemote::setPower(bool power)
    {
      if (power)
      {
        _fanOn = true;
        setSpeed(_speed);
      }
      else
      {
        _fanOn = false;
        send_command(StarFanRemoteCommandOff);
      }
    }

    void StarFanStatefulRemote::setSpeed(int speed)
    {
      const StarFanRemoteCommand speedCommands[] = {
          StarFanRemoteCommandSpd1,
          StarFanRemoteCommandSpd2,
          StarFanRemoteCommandSpd3,
          StarFanRemoteCommandSpd4,
          StarFanRemoteCommandSpd5,
          StarFanRemoteCommandSpd6};

      if (!speed)
      {
        setPower(false);
      }
      else
      {
        _speed = speed;
        if (_fanOn)
        {
          send_command(speedCommands[_speed - 1]);
        }
      }
    }

    void StarFanStatefulRemote::setDirection(bool ccw)
    {
      if (_fanOn)
      {
        if (_dirCcw != ccw)
        {
          _dirCcw = ccw;
          send_command(StarFanRemoteCommandDir);
        }
      }
    }

    void StarFanStatefulRemote::setLight(bool lightOn)
    {
      ESP_LOGD("StarFanStatefulRemote", "Set light: %s -> %s", _lightOn ? "ON" : "OFF", lightOn ? "ON" : "OFF");
      if (_lightOn != lightOn)
      {
        _lightOn = lightOn;
        send_command(StarFanRemoteCommandLight);
      }
    }

    void StarFanStatefulRemote::command_to_ook_packet(StarFanRemoteCommand cmd, uint8_t packet[], int *packet_len)
    {
      int bitIndex = 0;

#define ADD_HL(value)                                                                                                                           \
  packet[bitIndex / 8] = packet[bitIndex / 8] | ((value) ? (0x80 >> ((bitIndex) % 8)) : 0) & ((!(value)) ? ~(0x80 >> ((bitIndex) % 8)) : 0xff); \
  bitIndex++;
#define ADD_BIT(value) \
  ADD_HL(1);           \
  ADD_HL(0);           \
  ADD_HL(value);

      memset(packet, 0, 96);

      for (int j = 0; j < 4; j++)
      {
        for (int i = 0; i < 6; i++)
        {
          ADD_BIT(_address & (0x20 >> i));
        }

        for (int i = 0; i < 7; i++)
        {
          ADD_BIT(cmd & (0x40 >> i));
        }

        for (int i = 0; i < 34; i++)
        {
          ADD_HL(0);
        }
      }

      *packet_len = ((bitIndex - 1) / 8) + 1;
    }

    void StarFanStatefulRemote::send_command(StarFanRemoteCommand cmd)
    {
      // Generate OOK packet
      uint8_t packet[96];
      int packet_len = 96;
      command_to_ook_packet(cmd, packet, &packet_len);

      rfm69::QueuedTxPacket tx_packet{
          rfm69::QueuedTxPacket::RFM_TX_PACKET_TYPE_FIXED_LEN_RAW_OOK,
          3000,
          433920000,
          {},
          packet_len,
          1,
          6,
      };
      memcpy(tx_packet.packet, packet, packet_len);
      _transmitter->enqueue_tx_packet(tx_packet);
    }

    uint8_t StarFanStatefulRemote::getAddress()
    {
      return _address;
    }
  }
}
