#include "rfm69.h"
#include "aseer_radio_parser.h"
#include "esphome/core/log.h"

namespace esphome
{
  namespace rfm69
  {

    static const char *const TAG = "rfm69";

    static int __tracked_pin = 0;
    static void (*__installed_rx_interrupt_handler)(int state) = NULL;

    static QueueHandle_t _pendingPacketsQueue = nullptr;

    void __rfm69_pin_interrupt()
    {
      __installed_rx_interrupt_handler(digitalRead(__tracked_pin));
    }

    Rfm69::Rfm69(int pin_miso, int pin_mosi, int pin_sck, int pin_nss, int pin_dio2) : _pin_miso(pin_miso),
                                                                                       _pin_mosi(pin_mosi),
                                                                                       _pin_nss(pin_nss),
                                                                                       _pin_sck(pin_sck),
                                                                                       _pin_dio2(pin_dio2),
                                                                                       _listenerProtocol(RfmListenerProtocol::RfmListenerProtocolStandby)
    {
    }

    void Rfm69::setup()
    {
      ESP_LOGD(TAG, "RFM69 setup");
      pinMode(_pin_miso, INPUT);
      pinMode(_pin_mosi, OUTPUT);
      pinMode(_pin_nss, OUTPUT);
      pinMode(_pin_sck, OUTPUT);

      digitalWrite(_pin_nss, HIGH);
      digitalWrite(_pin_sck, LOW);
    }

    bool Rfm69::add_radio_protocol_listener(RfmListenerProtocol protocol,
                                            std::function<void(char *data, int len)> callback)
    {
      if (_listenerProtocol != RfmListenerProtocol::RfmListenerProtocolStandby)
      {
        ESP_LOGW(TAG, "RFM69 listener protocol already set");
        return false;
      }

      _listenerProtocol = protocol;
      _listenerCallbacks.push_back(callback);

      resume_listening();

      return true;
    }

    void Rfm69::resume_listening()
    {
      switch (_listenerProtocol)
      {
      case RfmListenerProtocol::RfmListenerProtocolAseer:
        resume_aseer_listening();
        break;

      default:
        set_mode(RfmMode::RfmModeStandby);
        break;
      }
    }

    void Rfm69::resume_aseer_listening()
    {
      if (!_pendingPacketsQueue)
      {
        _pendingPacketsQueue = xQueueCreate(4, sizeof(QueuedPacket));
      }

      aseer_set_output_queue(_pendingPacketsQueue);

      set_frequency(433420000);
      set_tx_lna_parameters(200, 1);
      set_rx_bandwidth(2, 0, 0);
      set_rx_noise_floor(20);
      set_rx_peak_mode(Rfm69::RfmPeakThresholdTypePeak, 0, 0);

      _receiverInterrupt = aseer_handle_state_interrupt;
      set_mode(Rfm69::RfmModeRxOokContAsync);
    }

    void Rfm69::start_transmit_mode(RfmMode mode)
    {
      set_mode(mode);
    }

    void Rfm69::end_transmit_mode()
    {
      resume_listening();
    }

    void Rfm69::xact(bool read, byte addr, byte buff[], byte len)
    {
      digitalWrite(_pin_nss, LOW);

      addr |= !read ? 0x80 : 0;
      byte_out(addr);

      if (!read)
      {
        for (int i = 0; i < len; i++)
        {
          byte_out(buff[i]);
        }
      }
      else
      {
        for (int i = 0; i < len; i++)
        {
          buff[i] = byte_in();
        }
      }

      digitalWrite(_pin_nss, HIGH);
      delayMicroseconds(RFM_SPI_DELAY);
    }

    void Rfm69::write_byte(byte addr, byte val)
    {
      xact(false, addr, &val, 1);
    }

    byte Rfm69::read_byte(byte addr)
    {
      byte b;
      xact(true, addr, &b, 1);
      return b;
    }

    void Rfm69::set_mode(RfmMode mode)
    {
      switch (mode)
      {
      case RfmModeRxOokContSync:
        write_byte(RFM_REG_REGOPMODE, 0x10);
        write_byte(RFM_REG_REGDATAMODUL, 0x48);
        pinMode(_pin_dio2, INPUT);
        install_rx_interrupt();
        break;

      case RfmModeRxOokContAsync:
        write_byte(RFM_REG_REGOPMODE, 0x10);
        write_byte(RFM_REG_REGDATAMODUL, 0x68);
        pinMode(_pin_dio2, INPUT);
        install_rx_interrupt();
        break;

      case RfmModeTxOokContAsync:
        detachInterrupt(digitalPinToInterrupt(_pin_dio2));
        write_byte(RFM_REG_REGOPMODE, 0x0c);
        write_byte(RFM_REG_REGDATAMODUL, 0x68); // + 0 for no shaping, 1 for cutoff=BR, 2 for cutoff with 2BR
        pinMode(_pin_dio2, OUTPUT);
        break;

      case RfmModeStandby:
        detachInterrupt(digitalPinToInterrupt(_pin_dio2));
        write_byte(RFM_REG_REGOPMODE, 0x04);
        break;

      case RfmModeTxOokPacket:
        detachInterrupt(digitalPinToInterrupt(_pin_dio2));
        write_byte(RFM_REG_REGOPMODE, 0x0c);
        write_byte(RFM_REG_DIO_MAPPING1, 0x04);
        write_byte(RFM_REG_REGDATAMODUL, 0x08); // + 0 for no shaping, 1 for cutoff=BR, 2 for cutoff with 2BR
        break;
      }
    }

    void Rfm69::loop()
    {
      QueuedPacket pendingBuffer;
      while (xQueueReceive(_pendingPacketsQueue, &pendingBuffer, 0) == pdTRUE)
      {
        for (auto callback : _listenerCallbacks)
        {
          callback((char *)pendingBuffer.data, pendingBuffer.len);
        }
      }
    }

    void Rfm69::install_rx_interrupt()
    {
      if (_receiverInterrupt)
      {
        __installed_rx_interrupt_handler = _receiverInterrupt;
        __tracked_pin = _pin_dio2;
        attachInterrupt(digitalPinToInterrupt(_pin_dio2), __rfm69_pin_interrupt, CHANGE);
      }
    }

    void Rfm69::set_packet_format(bool var_len, int dc_free, bool crc, bool crc_auto_clear, int addr_filtering, int preamble_size)
    {
      write_byte(RFM_REG_PACKETCONFIG1,
                 (var_len ? 0x80 : 0) |
                     (dc_free << 5) |
                     (crc ? 0x10 : 0) |
                     (crc_auto_clear ? 0x00 : 0x08) |
                     (addr_filtering << 1));

      write_byte(RFM_REG_PREAMBLEMSB, preamble_size >> 8);
      write_byte(RFM_REG_PREAMBLELSB, preamble_size & 0xFF);
    }

    void Rfm69::set_packet_sync_off()
    {
      Rfm69::write_byte(RFM_REG_SYNC_CONFIG, 0x00); // No sync
    }

    void Rfm69::set_frequency(long freq)
    {
      long freq_in_steps = freq / 61;
      write_byte(RFM_REG_FRF_BASE, freq_in_steps >> 16);
      write_byte(RFM_REG_FRF_BASE + 1, (freq_in_steps >> 8) & 0xff);
      write_byte(RFM_REG_FRF_BASE + 2, (freq_in_steps) & 0xff);
    }

    void Rfm69::set_tx_power_level(bool high_power, int power)
    {
      write_byte(RFM_REG_PA_LEVEL, (high_power ? 0x60 : 0x80) | power);
    }

    void Rfm69::set_rx_peak_mode(RfmPeakThresholdType peak_threshold, char threshold_step, char threshold_dec_period)
    {
      byte to_write = (peak_threshold << 6) | (threshold_step << 3) | (threshold_dec_period);
      write_byte(RFM_REG_REGOOKPEAK, to_write);
    }

    void Rfm69::set_rx_noise_floor(char noise_floor)
    {
      write_byte(RFM_REG_REGOOKFIX, noise_floor);
    }

    void Rfm69::set_rx_bandwidth(char dcc_freq, char bandwidth_mantissa, char bandwidth_exponent)
    {
      byte to_write = (dcc_freq << 5) | (bandwidth_mantissa << 3) | (bandwidth_exponent);
      write_byte(RFM_REG_REGRXBW, to_write);
    }

    void Rfm69::set_tx_lna_parameters(int impedance, int gainSelect)
    {
      byte to_write = ((impedance == 200 ? 1 : 0) << 7) | (gainSelect & 0x7);
      write_byte(RFM_REG_REGLNA, to_write);
    }

    void Rfm69::set_bitrate(long bitrate)
    {
      long br_denom = 32000000 / bitrate;
      write_byte(0x03, (br_denom >> 8) & 0xff);
      write_byte(0x04, (br_denom) & 0xff);
    }

    void Rfm69::send_fixed_len_packet(byte packet[], int len)
    {
      write_byte(RFM_REG_PAYLOAD_LENGTH, len); // No sync
      xact(false, 0, packet, len);
    }

    bool Rfm69::is_packet_sent()
    {
      byte irq2 = read_byte(RFM_REG_IRQ_FLAG2);
      return irq2 & 8;
    }
    void Rfm69::byte_out(byte b)
    {
      for (byte i = 0x80; i != 0; i = i >> 1)
      {
        digitalWrite(_pin_mosi, b & i ? HIGH : LOW);
        digitalWrite(_pin_sck, HIGH);
        delayMicroseconds(RFM_SPI_DELAY);
        digitalWrite(_pin_sck, LOW);
        delayMicroseconds(RFM_SPI_DELAY);
      }
    }

    byte Rfm69::byte_in()
    {
      byte ret = 0;
      for (byte i = 0; i < 8; i++)
      {
        digitalWrite(_pin_sck, HIGH);
        delayMicroseconds(RFM_SPI_DELAY);
        ret = (ret << 1) | digitalRead(_pin_miso);
        digitalWrite(_pin_sck, LOW);
        delayMicroseconds(RFM_SPI_DELAY);
      }

      return ret;
    }
  }
}
