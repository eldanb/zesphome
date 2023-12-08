#include "somfy_remote.h"
#include "esphome/core/log.h"
#include "esphome/components/rfm69/rfm69.h"
#include "rfm69_somfy_cover.h"
#include "somfy_remote.h"

#include <FS.h>

namespace esphome
{
  namespace rfm69_somfy_cover
  {
    using rfm69::Rfm69;

    SomfyRemote::SomfyRemote(Rfm69 *transmitter, long address)
        : _transmitter(transmitter), _address(address)
    {
      _pref = global_preferences->make_preference<uint32_t>(_address);
      load_rolling_code();
    }

    void SomfyRemote::init()
    {
      load_rolling_code();
    }

    void SomfyRemote::load_rolling_code()
    {
      _pref.load(&_rolling_code);
    }

    void SomfyRemote::increment_rolling_code()
    {
      _rolling_code++;
      _pref.save(&_rolling_code);
    }

    void SomfyRemote::compose_frame(byte ctrl, byte digitalOut[7])
    {
      digitalOut[0] = (_rolling_code + 0x32) & 0xff;
      digitalOut[1] = ctrl << 4;
      digitalOut[2] = _rolling_code >> 8;
      digitalOut[3] = _rolling_code & 0xff;
      digitalOut[4] = (_address >> 16) & 0xff;
      digitalOut[5] = (_address >> 8) & 0xff;
      digitalOut[6] = (_address) & 0xff;

      // Checksum
      byte cksum = 0;
      for (int i = 0; i < 7; i++)
      {
        cksum = cksum ^ digitalOut[i] ^ (digitalOut[i] >> 4);
      }
      digitalOut[1] = digitalOut[1] | (cksum & 0xf);

      // Obfuscate
      for (int i = 1; i < 7; i++)
      {
        digitalOut[i] = digitalOut[i] ^ digitalOut[i - 1];
      }
    }

    void SomfyRemote::frame_to_ook_packet(byte frame[7], byte packet[], int *packet_len)
    {
      memset(packet, 0, 66);

      // Generate high pulse and then silence -- wakeup
      for (int i = 0; i < 17; i++)
      {
        packet[i] = 0xff;
      }
      for (int i = 0; i < 15; i++)
      {
        packet[i + 17] = 0x0;
      }

      // Generate hardware and software sync
      packet[32] = 0xf0;
      packet[33] = 0xf0;
      packet[34] = 0xff;
      packet[35] = 0x00;

      int bit_idx = 281;

#define ___EMIT_OUTPUT_BIT(bval)                                               \
  packet[bit_idx >> 3] = packet[bit_idx >> 3] | (bval) << (7 - (bit_idx & 7)); \
  bit_idx++

      for (int fr_byte_idx = 0; fr_byte_idx < 7; fr_byte_idx++)
      {
        int frbitmask = 0x80;
        for (int fr_bit_idx = 0; fr_bit_idx < 8; fr_bit_idx++)
        {
          bool z = frame[fr_byte_idx] & frbitmask;
          ___EMIT_OUTPUT_BIT(!z);
          ___EMIT_OUTPUT_BIT(z);
          frbitmask = frbitmask >> 1;
        }
      }

      ___EMIT_OUTPUT_BIT(0);
      ___EMIT_OUTPUT_BIT(0);
      ___EMIT_OUTPUT_BIT(0);
      ___EMIT_OUTPUT_BIT(0);

#undef ___EMIT_OUTPUT_BIT

      int len = (bit_idx + 8) / 8;

      *packet_len = len;
    }

    void SomfyRemote::send_command(SomfyRemoteCommand cmd, int count)
    {
      ESP_LOGD(TAG, "Send command %d: rolling code %d", cmd, _rolling_code);

      // Prepare frame
      byte frame_content[7];
      compose_frame((byte)cmd, frame_content);

      // Generate OOK packet
      byte packet[66];
      int packet_len = 66;
      frame_to_ook_packet(frame_content, packet, &packet_len);

      // Bombs away
      for (int i = 0; i < 20; i++)
      {
        ESP_LOGD(TAG, "Send command iteration %d: setup transmitter", i);
        _transmitter->set_bitrate(1655);
        _transmitter->set_frequency(433420000);
        _transmitter->set_tx_power_level(true, 31);
        _transmitter->set_packet_format(false, 0, false, true, 0, 0);
        _transmitter->set_packet_sync_off();
        _transmitter->set_mode(Rfm69::RfmModeTxOokPacket);

        ESP_LOGD(TAG, "Send command iteration %d: send packet len = %d", i, packet_len + 1);
        _transmitter->send_fixed_len_packet(packet, packet_len + 1);

        ESP_LOGD(TAG, "Send command iteration %d: wait transmit done", i);

        int delayIteration;
        for (delayIteration = 0; delayIteration < 100; delayIteration++)
        {
          if (_transmitter->is_packet_sent())
          {
            break;
          }

          delay(5);
        }
        ESP_LOGD(TAG, "Command send complete: %d delay iterations", delayIteration);
      }

      _transmitter->set_mode(Rfm69::RfmModeStandby);
      ESP_LOGD(TAG, "Transmitter in standby");

      increment_rolling_code();
    }

    long SomfyRemote::getAddress()
    {
      return _address;
    }

    long SomfyRemote::getRollingCode()
    {
      return _rolling_code;
    }
  }
}