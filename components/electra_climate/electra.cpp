#include "electra.h"
#include "esphome/core/log.h"
#include "IRelectra.h"

namespace esphome
{
  namespace electra_climate
  {

    static const char *const TAG = "electra.climate";

    climate::ClimateTraits ElectraClimate::traits()
    {
      auto traits = climate::ClimateTraits();
      traits.set_visual_min_temperature(16);
      traits.set_visual_max_temperature(31);
      traits.set_visual_temperature_step(1.0f);
      traits.set_supported_modes({climate::CLIMATE_MODE_OFF,
                                  climate::CLIMATE_MODE_COOL,
                                  climate::CLIMATE_MODE_HEAT,
                                  climate::CLIMATE_MODE_DRY,
                                  climate::CLIMATE_MODE_FAN_ONLY});

      // Default to only 3 levels in ESPHome even if most unit supports 4. The 3rd level is not used.
      traits.set_supported_fan_modes(
          {climate::CLIMATE_FAN_AUTO, climate::CLIMATE_FAN_LOW, climate::CLIMATE_FAN_MEDIUM, climate::CLIMATE_FAN_HIGH});

      // TODO add swing modes boolean?
      // traits.set_supported_swing_modes({climate::CLIMATE_SWING_OFF, climate::CLIMATE_SWING_BOTH,
      //                                   climate::CLIMATE_SWING_VERTICAL, climate::CLIMATE_SWING_HORIZONTAL});

      traits.set_supported_presets({});

      return traits;
    }

    void ElectraClimate::transmit_state()
    {
      auto current_state_power = this->mode != climate::CLIMATE_MODE_OFF;
      auto power_toggle = current_state_power != _last_transmitted_power;

      IRElectraMode electra_mode;
      switch (this->mode)
      {
      case climate::CLIMATE_MODE_COOL:
        electra_mode = IRElectraModeCool;
        break;
      case climate::CLIMATE_MODE_HEAT:
        electra_mode = IRElectraModeHeat;
        break;
      case climate::CLIMATE_MODE_DRY:
        electra_mode = IRElectraModeDry;
        break;
      case climate::CLIMATE_MODE_FAN_ONLY:
        electra_mode = IRElectraModeFan;
        break;
      case climate::CLIMATE_MODE_OFF:
      default:
        electra_mode = IRElectraModeAuto; // Irrelevant when power is off
        break;
      }

      IRElectraFan electra_fan;
      switch (this->fan_mode.value_or(climate::CLIMATE_FAN_AUTO))
      {
      case climate::CLIMATE_FAN_LOW:
        electra_fan = IRElectraFanLow;
        break;
      case climate::CLIMATE_FAN_MEDIUM:
        electra_fan = IRElectraFanMedium;
        break;
      case climate::CLIMATE_FAN_HIGH:
        electra_fan = IRElectraFanHigh;
        break;
      case climate::CLIMATE_FAN_AUTO:
      default:
        electra_fan = IRElectraFanAuto;
        break;
      }

      IRelectra ir_electra;
      auto signal = ir_electra.generateSignal(
          power_toggle,
          electra_mode,
          electra_fan, this->target_temperature,
          false,
          false);

      auto transmit = this->transmitter_->transmit();
      auto *data = transmit.get_data();

      data->set_carrier_frequency(38000);
      for (int i = 0; i < signal.size(); i += 2)
      {
        data->mark(signal[i]);
        data->space(signal[i + 1]);
      }
      transmit.perform();

      _last_transmitted_power = current_state_power;
    }
  } // namespace mitsubishi
} // namespace esphome
