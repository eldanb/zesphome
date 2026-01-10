#pragma once

#include <list>
#include <functional>
#include <Arduino.h>

#include "esphome/core/component.h"

#define RFM_REG_REGOPMODE 0x01
#define RFM_REG_REGDATAMODUL 0x02
#define RFM_REG_REGOSC1 0x0a
#define RFM_REG_PACKETCONFIG1 0x37
#define RFM_REG_PREAMBLEMSB 0x2C
#define RFM_REG_PREAMBLELSB 0x2D
#define RFM_REG_SYNC_CONFIG 0x2E
#define RFM_REG_PA_LEVEL 0x11
#define RFM_REG_PAYLOAD_LENGTH 0x38
#define RFM_REG_FRF_BASE 0x07
#define RFM_REG_DIO_MAPPING1 0x25
#define RFM_REG_IRQ_FLAG2 0x28
#define RFM_REG_REGRXBW 0x19
#define RFM_REG_REGOOKPEAK 0x1B
#define RFM_REG_REGOOKFIX 0x1D
#define RFM_REG_REGLNA 0x18

#define RFM_SPI_DELAY 10

#define RFM_SENSOR_MAX_RX_BUFFER_SIZE 16
#define RFM_MAX_TX_PACKET_SIZE 128

namespace esphome
{
  namespace rfm69
  {

    struct QueuedPacket
    {
      uint8_t data[RFM_SENSOR_MAX_RX_BUFFER_SIZE];
      int len;
    };

    struct QueuedTxPacket
    {
      enum
      {
        RFM_TX_PACKET_TYPE_FIXED_LEN_RAW_OOK = 0
      } type;
      int bitRate;
      long frequency;

      byte packet[RFM_MAX_TX_PACKET_SIZE];
      int len;
    };

    class Rfm69 : public Component
    {
    public:
      Rfm69(int pin_miso, int pin_mosi, int pin_sck, int pin_nss, int pin_dio2);

      void setup() override;
      void xact(bool read, byte addr, byte buff[], byte len);
      void write_byte(byte addr, byte val);
      byte read_byte(byte addr);

      enum RfmMode
      {
        RfmModeStandby,
        RfmModeRxOokContSync,
        RfmModeRxOokContAsync,
        RfmModeTxOokContAsync,
        RfmModeTxOokPacket
      };

      enum RfmListenerProtocol
      {
        RfmListenerProtocolStandby,
        RfmListenerProtocolAseer
      };

      enum RfmPeakThresholdType
      {
        RfmPeakThresholdTypeFixed,
        RfmPeakThresholdTypePeak,
        RfmPeakThresholdTypeAverage,
        RfmPeakThresholdTypeReserved
      };

      bool add_radio_protocol_listener(RfmListenerProtocol protocol,
                                       std::function<void(char *data, int len)> callback);
      void enqueue_tx_packet(const QueuedTxPacket &packet);

      void loop() override;

    private:
      bool tx_queued_packet(QueuedTxPacket *packet);

      void start_transmit_mode(RfmMode mode);
      void end_transmit_mode();

      void set_mode(RfmMode mode);
      void set_packet_format(bool var_len, int dc_free, bool crc, bool crc_auto_clear, int addr_filtering, int preamble_size);
      void set_packet_sync_off();
      void set_frequency(long freq);
      void set_bitrate(long bitrate);

      void set_rx_peak_mode(RfmPeakThresholdType peak_threshold, char threshold_step, char threshold_dec_period);
      void set_rx_noise_floor(char noise_floor);
      void set_rx_bandwidth(char dcc_freq, char bandwidth_mantissa, char bandwidth_exponent);
      void set_tx_lna_parameters(int impedance, int gainSelect);
      void set_tx_power_level(bool high_power, int power);
      void send_fixed_len_packet(byte packet[], int len);
      bool is_packet_sent();

      void resume_listening();

      void resume_aseer_listening();
      void install_rx_interrupt();

      void byte_out(byte b);
      byte byte_in();

      int _pin_miso;
      int _pin_mosi;
      int _pin_nss;
      int _pin_sck;
      int _pin_dio2;

      RfmListenerProtocol _listenerProtocol;
      std::list<std::function<void(char *data, int len)>> _listenerCallbacks;

      int _sendInProgress;
      QueueHandle_t _pendingTxPacketsQueue;

      void (*_receiverInterrupt)(int state);
    };
  }
}
