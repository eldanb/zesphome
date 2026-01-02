#include "rfm69_binary_sensor.h"
#include "esphome/core/log.h"

namespace esphome
{
  namespace rfm69_binary_sensor
  {

    static const char *const TAG = "rfm69.binary_sensor";

    Rfm69BinarySensor::Rfm69BinarySensor(Rfm69 *rfm,
                                         std::vector<uint8_t> &&on_packet,
                                         std::vector<uint8_t> &&off_packet)
        : rfm69_(rfm),
          on_packet_(on_packet),
          off_packet_(off_packet)
    {
    }

    void Rfm69BinarySensor::setup()
    {
      rfm69_->add_radio_protocol_listener(Rfm69::RfmListenerProtocolAseer,
                                          [this](char *data, int len)
                                          {
                                            ESP_LOGD(TAG, "RFM69 Binary Sensor received packet of len %d; %02x %02x %02x....", len, data[0], data[1], data[2]);
                                            if (len == on_packet_.size() &&
                                                !memcmp(data, on_packet_.data(), len))
                                            {
                                              this->publish_state(true);
                                            }
                                            else if (len == off_packet_.size() &&
                                                     !memcmp(data, off_packet_.data(), len))
                                            {
                                              this->publish_state(false);
                                            }
                                          });

      disable_loop();
    }

    void Rfm69BinarySensor::dump_config()
    {
      LOG_BINARY_SENSOR("", "RFM69 Binary Sensor", this);
    }

    void Rfm69BinarySensor::loop()
    {
    }
  } // namespace gpio
} // namespace esphome
