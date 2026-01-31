#include "pellini_remote.h"
#include "esphome/components/rfm69/rfm69.h"

namespace esphome
{
  namespace rfm69_pellini_cover
  {
    using rfm69::Rfm69;

    PelliniRemote::PelliniRemote(Rfm69 *transmitter, uint8_t address)
        : _transmitter(transmitter), _address(address)
    {
    }

    void PelliniRemote::init()
    {
    }

    static const int crcTable[] = {
        0x0000, 0x8005, 0x800F, 0x000A, 0x801B, 0x001E, 0x0014, 0x8011, 0x8033, 0x0036, 0x003C, 0x8039, 0x0028, 0x802D, 0x8027, 0x0022,
        0x8063, 0x0066, 0x006C, 0x8069, 0x0078, 0x807D, 0x8077, 0x0072, 0x0050, 0x8055, 0x805F, 0x005A, 0x804B, 0x004E, 0x0044, 0x8041,
        0x80C3, 0x00C6, 0x00CC, 0x80C9, 0x00D8, 0x80DD, 0x80D7, 0x00D2, 0x00F0, 0x80F5, 0x80FF, 0x00FA, 0x80EB, 0x00EE, 0x00E4, 0x80E1,
        0x00A0, 0x80A5, 0x80AF, 0x00AA, 0x80BB, 0x00BE, 0x00B4, 0x80B1, 0x8093, 0x0096, 0x009C, 0x8099, 0x0088, 0x808D, 0x8087, 0x0082,
        0x8183, 0x0186, 0x018C, 0x8189, 0x0198, 0x819D, 0x8197, 0x0192, 0x01B0, 0x81B5, 0x81BF, 0x01BA, 0x81AB, 0x01AE, 0x01A4, 0x81A1,
        0x01E0, 0x81E5, 0x81EF, 0x01EA, 0x81FB, 0x01FE, 0x01F4, 0x81F1, 0x81D3, 0x01D6, 0x01DC, 0x81D9, 0x01C8, 0x81CD, 0x81C7, 0x01C2,
        0x0140, 0x8145, 0x814F, 0x014A, 0x815B, 0x015E, 0x0154, 0x8151, 0x8173, 0x0176, 0x017C, 0x8179, 0x0168, 0x816D, 0x8167, 0x0162,
        0x8123, 0x0126, 0x012C, 0x8129, 0x0138, 0x813D, 0x8137, 0x0132, 0x0110, 0x8115, 0x811F, 0x011A, 0x810B, 0x010E, 0x0104, 0x8101,
        0x8303, 0x0306, 0x030C, 0x8309, 0x0318, 0x831D, 0x8317, 0x0312, 0x0330, 0x8335, 0x833F, 0x033A, 0x832B, 0x032E, 0x0324, 0x8321,
        0x0360, 0x8365, 0x836F, 0x036A, 0x837B, 0x037E, 0x0374, 0x8371, 0x8353, 0x0356, 0x035C, 0x8359, 0x0348, 0x834D, 0x8347, 0x0342,
        0x03C0, 0x83C5, 0x83CF, 0x03CA, 0x83DB, 0x03DE, 0x03D4, 0x83D1, 0x83F3, 0x03F6, 0x03FC, 0x83F9, 0x03E8, 0x83ED, 0x83E7, 0x03E2,
        0x83A3, 0x03A6, 0x03AC, 0x83A9, 0x03B8, 0x83BD, 0x83B7, 0x03B2, 0x0390, 0x8395, 0x839F, 0x039A, 0x838B, 0x038E, 0x0384, 0x8381,
        0x0280, 0x8285, 0x828F, 0x028A, 0x829B, 0x029E, 0x0294, 0x8291, 0x82B3, 0x02B6, 0x02BC, 0x82B9, 0x02A8, 0x82AD, 0x82A7, 0x02A2,
        0x82E3, 0x02E6, 0x02EC, 0x82E9, 0x02F8, 0x82FD, 0x82F7, 0x02F2, 0x02D0, 0x82D5, 0x82DF, 0x02DA, 0x82CB, 0x02CE, 0x02C4, 0x82C1,
        0x8243, 0x0246, 0x024C, 0x8249, 0x0258, 0x825D, 0x8257, 0x0252, 0x0270, 0x8275, 0x827F, 0x027A, 0x826B, 0x026E, 0x0264, 0x8261,
        0x0220, 0x8225, 0x822F, 0x022A, 0x823B, 0x023E, 0x0234, 0x8231, 0x8213, 0x0216, 0x021C, 0x8219, 0x0208, 0x820D, 0x8207, 0x0202};

    static void crc_init(int *crc)
    {
      *crc = 0;
    }

    static void crc_update(int *crc, uint8_t bt)
    {
      char idx = (bt ^ (*crc >> 8)) & 0xFF;
      *crc = crcTable[idx] ^ (*crc << 8);
    }

    void PelliniRemote::compose_frame(uint8_t ctrl, uint8_t digitalOut[6])
    {
      digitalOut[0] = _address;
      digitalOut[1] = ctrl;
      digitalOut[2] = 0x27; // What are these?
      digitalOut[3] = 0x48; // What are these

      int crc;
      crc_init(&crc);
      for (int i = 0; i < 4; i++)
        crc_update(&crc, digitalOut[i]);

      digitalOut[4] = (crc >> 8) & 0xff;
      digitalOut[5] = (crc) & 0xff;

      ESP_LOGD("PelliniRemote", "Composed frame: %02x %02x %02x %02x %02x %02x",
               digitalOut[0], digitalOut[1], digitalOut[2],
               digitalOut[3], digitalOut[4], digitalOut[5]);
    }

    void PelliniRemote::frame_to_ook_packet(uint8_t frame[6], uint8_t packet[], int *packet_len)
    {
      memset(packet, 0, 66);

      packet[0] = 7;    // 3 High bits
      packet[1] = 0xc2; // 2 High bits
                        // 4 low bits
                        // 1 HL
      packet[2] = 0xaa; // 4 x HL
      packet[3] = 0xaa; // 4 x HL
      packet[4] = 0xa8; // 3 x HL
                        // 2 Low
      packet[5] = 0;    // 8 Low

      int packet_idx = 6;
      int bitbuffer = 0;
      int bitsinbuffer = 0;

      // And now, emit the packet. Send LSB first; a "1" becomes 110, a "0" becomes "100"
      for (int i = 0; i < 6; i++)
      {
        uint8_t fr = frame[i];
        for (int bitidx = 0; bitidx < 8; bitidx++)
        {
          uint8_t bitbits = fr & 1 ? 6 : 4;
          bitbuffer = (bitbuffer << 3) | bitbits;
          bitsinbuffer += 3;
          if (bitsinbuffer >= 8)
          {
            bitsinbuffer -= 8;
            packet[packet_idx++] = (bitbuffer >> bitsinbuffer) & 0xff;
          }

          fr = fr >> 1;
        }
      }

      // Flush unwritten bits + an additional "1"
      bitbuffer = (bitbuffer << 1) | 1;
      bitsinbuffer++;
      packet[packet_idx++] = bitbuffer << (8 - bitsinbuffer);

      *packet_len = packet_idx;
    }

    void PelliniRemote::send_command(PelliniRemoteCommand cmd, int count, int last_delay)
    {
      uint8_t frame_content[6];
      compose_frame((uint8_t)cmd, frame_content);

      // Generate OOK packet
      uint8_t packet[66];
      int packet_len = 66;
      frame_to_ook_packet(frame_content, packet, &packet_len);

      rfm69::QueuedTxPacket tx_packet;
      tx_packet.type = rfm69::QueuedTxPacket::RFM_TX_PACKET_TYPE_FIXED_LEN_RAW_OOK;
      tx_packet.bitRate = 2500;
      tx_packet.frequency = 433920000;
      tx_packet.minDelay = 20;
      tx_packet.count = 1;
      memcpy(tx_packet.packet, packet, packet_len);
      tx_packet.len = packet_len;
      _transmitter->enqueue_tx_packet(tx_packet);

      if (count > 1)
      {
        count--;

        // Inhibit wakeup pulse from further packets
        tx_packet.packet[0] = 0;
        tx_packet.packet[1] = 2;

        if (count > 1)
        {
          tx_packet.count = count - 1;
          _transmitter->enqueue_tx_packet(tx_packet);
        }

        tx_packet.count = 1;
        tx_packet.minDelay = last_delay;
        _transmitter->enqueue_tx_packet(tx_packet);
      }
    }

    void PelliniRemote::send_command_for_duration(PelliniRemoteCommand cmd, int duration)
    {
      if (duration <= 2000)
      {
        send_command(cmd, duration / 100);
      }
      else
      {
        send_command(cmd, 20, duration - 2000);
        send_command(cmd, 10);
      }
    }

    int PelliniRemote::getAddress()
    {
      return _address;
    }

  }
}
