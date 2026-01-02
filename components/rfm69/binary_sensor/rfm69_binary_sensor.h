#pragma once

#include "esphome/core/component.h"
#include "esphome/core/hal.h"
#include "esphome/core/helpers.h"
#include "esphome/components/binary_sensor/binary_sensor.h"
#include "esphome/components/rfm69/rfm69.h"

#include <vector>
#define MAX_PACKET_SIZE 6

namespace esphome
{
  namespace rfm69_binary_sensor
  {
    using esphome::rfm69::Rfm69;

    class Rfm69BinarySensor : public binary_sensor::BinarySensor, public Component
    {
    public:
      Rfm69BinarySensor(Rfm69 *rfm,
                        std::vector<uint8_t> &&on_packet,
                        std::vector<uint8_t> &&off_packet);

      void setup() override;
      void dump_config() override;
      void loop() override;

    protected:
      std::vector<uint8_t> on_packet_;
      std::vector<uint8_t> off_packet_;

      Rfm69 *rfm69_{nullptr};
    };

  } // namespace gpio
} // namespace esphome
