#include "star_fan.h"
#include "esphome/core/log.h"

namespace esphome
{
  namespace rfm69_star_fan
  {

    static const char *const TAG = "rfm69_star_fan";

    void Rfm69StarFan::control(const fan::FanCall &call)
    {
      ESP_LOGD(TAG, "Fan control call %s: state=%s speed=%d oscillating=%s direction=%s preset_mode=%s",
               this->get_name().c_str(),
               call.get_state().has_value() ? (*call.get_state() ? "ON" : "OFF") : "N/A",
               call.get_speed().value_or(-1),
               call.get_oscillating().has_value() ? (*call.get_oscillating() ? "ON" : "OFF") : "N/A",
               call.get_direction().has_value() ? (call.get_direction() == fan::FanDirection::REVERSE ? "REVERSE" : "FORWARD") : "N/A",
               call.get_preset_mode() != nullptr ? call.get_preset_mode() : "N/A");

      if (call.get_state().has_value())
      {
        _remote.setPower(*call.get_state());
      }
      if (call.get_speed().has_value())
      {
        _remote.setSpeed(*call.get_speed());
      }
      if (call.get_direction().has_value())
      {
        _remote.setDirection(*call.get_direction() == fan::FanDirection::REVERSE);
      }

      speed = _remote.getSpeed();
      direction = _remote.getDirection() ? fan::FanDirection::REVERSE : fan::FanDirection::FORWARD;
      state = _remote.getPower();

      publish_state();
    }

    void Rfm69StarFan::set_light(bool light)
    {
      _remote.setLight(light);
      if (light_switch_ != nullptr)
      {
        light_switch_->publish_state(light);
      }
    }

    void Rfm69StarFan::reset()
    {
      _remote.reset();
    }

    void Rfm69StarFanLightSwitch::write_state(bool state)
    {
      this->parent_->set_light(state);
      this->state = state;
      publish_state(state);
    }

    void Rfm69StarFanResetButton::press_action()
    {
      this->parent_->reset();
    }
  } // namespace rfm69_star_fan
} // namespace esphome
