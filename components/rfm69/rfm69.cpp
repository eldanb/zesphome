#include "rfm69.h"
#include "aseer_radio_parser.h"
#include "esphome/core/log.h"
#include "driver/gpio.h"
#include "esp_rom_sys.h"
#include "freertos/task.h"
#include "esp_timer.h"

namespace esphome
{
  namespace rfm69
  {

    static const char *const TAG = "rfm69";

    static int __tracked_pin = 0;
    static std::function<void(int)> __installed_rx_interrupt_handler = nullptr;

    static QueueHandle_t _pendingRxPacketsQueue = nullptr;

    void IRAM_ATTR __rfm69_pin_interrupt(void *arg)
    {
      __installed_rx_interrupt_handler(gpio_get_level((gpio_num_t)(int)arg));
    }

    Rfm69::Rfm69(int pin_miso, int pin_mosi, int pin_sck, int pin_nss, int pin_dio2) : _pin_miso(pin_miso),
                                                                                       _pin_mosi(pin_mosi),
                                                                                       _pin_nss(pin_nss),
                                                                                       _pin_sck(pin_sck),
                                                                                       _pin_dio2(pin_dio2),
                                                                                       _listenerProtocol(RfmListenerProtocol::RfmListenerProtocolStandby),
                                                                                       _sendInProgress(0),
                                                                                       _currentMode(RfmMode::RfmModeStandby),
                                                                                       _sleepUntil(0),
                                                                                       _transmitterBusy(false),
                                                                                       _receiverInterrupt(nullptr)
    {
      _pendingTxPacketsQueue = xQueueCreate(256, sizeof(QueuedTxPacket));
      _currentTxPacket.count = 0;
    }

    void Rfm69::setup()
    {
      ESP_LOGD(TAG, "RFM69 setup");

      gpio_config_t io_conf;
      io_conf.intr_type = GPIO_INTR_DISABLE;
      io_conf.mode = GPIO_MODE_OUTPUT; // or GPIO_MODE_INPUT
      io_conf.pin_bit_mask = (1ULL << _pin_mosi) | (1ULL << _pin_sck) | (1ULL << _pin_nss) | (1ULL << _pin_dio2);
      io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
      io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
      gpio_config(&io_conf);

      io_conf.mode = GPIO_MODE_INPUT; // or GPIO_MODE_INPUT
      io_conf.pin_bit_mask = (1ULL << _pin_miso);
      gpio_config(&io_conf);

      gpio_set_level((gpio_num_t)_pin_nss, 1);
      gpio_set_level((gpio_num_t)_pin_sck, 0);

      gpio_install_isr_service(0);

      ESP_LOGD(TAG, "RFM69 dump GPIO configuration:");
      gpio_dump_io_configuration(stdout, (1ULL << 15) | (1ULL << 16) | (1ULL << 17) | (1ULL << 18) | (1ULL << 19));
    }

    void Rfm69::enqueue_tx_packet(const QueuedTxPacket &packet)
    {
      xQueueSendToBack(_pendingTxPacketsQueue, &packet, 0);
    }

    void Rfm69::add_transmit_busy_change_callback(std::function<void()> callback)
    {
      _onTransmitBusyChangeCallbacks.push_back(callback);
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
      if (!_pendingRxPacketsQueue)
      {
        _pendingRxPacketsQueue = xQueueCreate(4, sizeof(QueuedPacket));
      }

      aseer_set_output_queue(_pendingRxPacketsQueue);

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

    void Rfm69::xact(bool read, uint8_t addr, uint8_t buff[], uint8_t len)
    {
      gpio_set_level((gpio_num_t)_pin_nss, 0);

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

      gpio_set_level((gpio_num_t)_pin_nss, 1);
      esp_rom_delay_us(RFM_SPI_DELAY);
    }

    void Rfm69::write_byte(uint8_t addr, uint8_t val)
    {
      xact(false, addr, &val, 1);
    }

    uint8_t Rfm69::read_byte(uint8_t addr)
    {
      uint8_t b;
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
        gpio_set_direction((gpio_num_t)_pin_dio2, GPIO_MODE_INPUT);
        install_rx_interrupt();
        break;

      case RfmModeRxOokContAsync:
        write_byte(RFM_REG_REGOPMODE, 0x10);
        write_byte(RFM_REG_REGDATAMODUL, 0x68);
        gpio_set_direction((gpio_num_t)_pin_dio2, GPIO_MODE_INPUT);
        install_rx_interrupt();
        break;

      case RfmModeTxOokContAsync:
        gpio_isr_handler_remove((gpio_num_t)_pin_dio2);
        write_byte(RFM_REG_REGOPMODE, 0x0c);
        write_byte(RFM_REG_REGDATAMODUL, 0x68); // + 0 for no shaping, 1 for cutoff=BR, 2 for cutoff with 2BR
        gpio_set_direction((gpio_num_t)_pin_dio2, GPIO_MODE_OUTPUT);
        break;

      case RfmModeStandby:
        gpio_isr_handler_remove((gpio_num_t)_pin_dio2);
        write_byte(RFM_REG_REGOPMODE, 0x04);
        break;

      case RfmModeTxOokPacket:
        gpio_isr_handler_remove((gpio_num_t)_pin_dio2);
        write_byte(RFM_REG_REGOPMODE, 0x0c);
        write_byte(RFM_REG_DIO_MAPPING1, 0x04);
        write_byte(RFM_REG_REGDATAMODUL, 0x08); // + 0 for no shaping, 1 for cutoff=BR, 2 for cutoff with 2BR
        break;
      }

      _currentMode = mode;
    }

    void Rfm69::loop()
    {
      QueuedPacket pendingBuffer;
      while (xQueueReceive(_pendingRxPacketsQueue, &pendingBuffer, 0) == pdTRUE)
      {
        for (auto callback : _listenerCallbacks)
        {
          callback((char *)pendingBuffer.data, pendingBuffer.len);
        }
      }

      if (!is_packet_sending_in_progress())
      {
        bool isTransmitterBusy = dequeue_and_tx_packet();
        if (isTransmitterBusy != _transmitterBusy)
        {
          _transmitterBusy = isTransmitterBusy;
          for (auto callback : _onTransmitBusyChangeCallbacks)
          {
            callback();
          }
        }
      }
    }

    bool Rfm69::dequeue_and_tx_packet()
    {
      if (_currentTxPacket.count == 0)
      {
        xQueueReceive(_pendingTxPacketsQueue, &_currentTxPacket, 0);
      }

      if (_currentTxPacket.count > 0)
      {
        ESP_LOGD(TAG, "Got TX packet, count %d", _currentTxPacket.count);
        _currentTxPacket.count--;
        if (tx_queued_packet(&_currentTxPacket))
        {
          _sendInProgress = esp_timer_get_time() / 1000;
          return true;
        }
      }

      return false;
    }

    bool Rfm69::is_packet_sending_in_progress()
    {
      if (_sendInProgress > 0)
      {
        if (is_packet_sent())
        {
          ESP_LOGD(TAG, "RFM69 transmit done");
          _sendInProgress = 0;
          _sleepUntil = (esp_timer_get_time() / 1000) + _currentTxPacket.minDelay;
          end_transmit_mode();
        }
        else if (_sendInProgress < (esp_timer_get_time() / 1000) - 5000)
        {
          ESP_LOGD(TAG, "RFM69 transmit timeout");
          end_transmit_mode();
          _sendInProgress = 0;
          _sleepUntil = 0;
          return false;
        }
        else
        {
          return true;
        }
      }

      int sleepAmount = _sleepUntil - (esp_timer_get_time() / 1000);
      if (sleepAmount > 40)
      {
        return true;
      }
      else
      {
        if (sleepAmount > 0)
        {
          vTaskDelay(pdMS_TO_TICKS(sleepAmount));
        }
        _sleepUntil = 0;

        return false;
      }
    }

    bool Rfm69::tx_queued_packet(QueuedTxPacket *packet)
    {
      ESP_LOGD(TAG, "RFM69 transmit packet type=%d", packet->type);

      switch (packet->type)
      {
      case QueuedTxPacket::RFM_TX_PACKET_TYPE_FIXED_LEN_RAW_OOK:
        set_bitrate(packet->bitRate);
        set_frequency(packet->frequency);
        set_tx_power_level(true, 31);
        set_packet_format(false, 0, false, true, 0, 0);
        set_packet_sync_off();
        start_transmit_mode(RfmModeTxOokPacket);

        send_fixed_len_packet(packet->packet, packet->len);
        return true;
        break;

      default:
        ESP_LOGW(TAG, "RFM69 transmit unknown packet type %d", packet->type);
        break;
      }

      return false;
    }

    void Rfm69::install_rx_interrupt()
    {
      if (_receiverInterrupt)
      {
        __installed_rx_interrupt_handler = _receiverInterrupt;
        __tracked_pin = _pin_dio2;
        gpio_set_intr_type((gpio_num_t)_pin_dio2, GPIO_INTR_ANYEDGE);
        gpio_isr_handler_add((gpio_num_t)_pin_dio2, __rfm69_pin_interrupt, (void *)__tracked_pin);
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
      uint8_t to_write = (peak_threshold << 6) | (threshold_step << 3) | (threshold_dec_period);
      write_byte(RFM_REG_REGOOKPEAK, to_write);
    }

    void Rfm69::set_rx_noise_floor(char noise_floor)
    {
      write_byte(RFM_REG_REGOOKFIX, noise_floor);
    }

    void Rfm69::set_rx_bandwidth(char dcc_freq, char bandwidth_mantissa, char bandwidth_exponent)
    {
      uint8_t to_write = (dcc_freq << 5) | (bandwidth_mantissa << 3) | (bandwidth_exponent);
      write_byte(RFM_REG_REGRXBW, to_write);
    }

    void Rfm69::set_tx_lna_parameters(int impedance, int gainSelect)
    {
      uint8_t to_write = ((impedance == 200 ? 1 : 0) << 7) | (gainSelect & 0x7);
      write_byte(RFM_REG_REGLNA, to_write);
    }

    void Rfm69::set_bitrate(long bitrate)
    {
      long br_denom = 32000000 / bitrate;
      write_byte(0x03, (br_denom >> 8) & 0xff);
      write_byte(0x04, (br_denom) & 0xff);
    }

    void Rfm69::send_fixed_len_packet(uint8_t packet[], int len)
    {
      write_byte(RFM_REG_PAYLOAD_LENGTH, len); // No sync
      xact(false, 0, packet, len);
    }

    bool Rfm69::is_packet_sent()
    {
      uint8_t irq2 = read_byte(RFM_REG_IRQ_FLAG2);
      return irq2 & 8;
    }

    void Rfm69::byte_out(uint8_t b)
    {
      for (uint8_t i = 0x80; i != 0; i = i >> 1)
      {
        gpio_set_level((gpio_num_t)_pin_mosi, b & i ? 1 : 0);
        gpio_set_level((gpio_num_t)_pin_sck, 1);
        esp_rom_delay_us(RFM_SPI_DELAY);
        gpio_set_level((gpio_num_t)_pin_sck, 0);
        esp_rom_delay_us(RFM_SPI_DELAY);
      }
    }

    uint8_t Rfm69::byte_in()
    {
      uint8_t ret = 0;
      for (uint8_t i = 0; i < 8; i++)
      {
        gpio_set_level((gpio_num_t)_pin_sck, 1);
        esp_rom_delay_us(RFM_SPI_DELAY);
        ret = (ret << 1) | gpio_get_level((gpio_num_t)_pin_miso);
        gpio_set_level((gpio_num_t)_pin_sck, 0);
        esp_rom_delay_us(RFM_SPI_DELAY);
      }

      return ret;
    }
  }
}
