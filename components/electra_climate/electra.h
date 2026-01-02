#pragma once

#include "esphome/components/climate_ir/climate_ir.h"

#include <cinttypes>

namespace esphome
{
  namespace electra_climate
  {

    class ElectraClimate : public climate_ir::ClimateIR
    {
    public:
      ElectraClimate()
          : climate_ir::ClimateIR(16, 31, 1.0f, true, true,
                                  {climate::CLIMATE_FAN_AUTO, climate::CLIMATE_FAN_LOW, climate::CLIMATE_FAN_MIDDLE,
                                   climate::CLIMATE_FAN_MEDIUM, climate::CLIMATE_FAN_HIGH, climate::CLIMATE_FAN_QUIET},
                                  {climate::CLIMATE_SWING_OFF, climate::CLIMATE_SWING_BOTH, climate::CLIMATE_SWING_VERTICAL,
                                   climate::CLIMATE_SWING_HORIZONTAL},
                                  {climate::CLIMATE_PRESET_NONE, climate::CLIMATE_PRESET_ECO, climate::CLIMATE_PRESET_BOOST,
                                   climate::CLIMATE_PRESET_SLEEP}) {}

    protected:
      // Transmit via IR the state of this climate controller.
      void transmit_state() override;

      climate::ClimateTraits traits() override;

    private:
      bool _last_transmitted_power = false;
    };

  } // namespace mitsubishi
} // namespace esphome
