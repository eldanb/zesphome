#pragma once

#include "esphome/core/component.h"
#include "esphome/components/fan/fan.h"
#include "star_fan_stateful_remote.h"
#include "esphome/components/switch/switch.h"
#include "esphome/components/button/button.h"

namespace esphome
{
  namespace rfm69_star_fan
  {

    class Rfm69StarFan : public Component, public fan::Fan
    {
      SUB_SWITCH(light)
      SUB_BUTTON(reset)

    public:
      Rfm69StarFan(rfm69::Rfm69 *rfm, unsigned long address) : _remote(rfm, address) {}
      fan::FanTraits get_traits() override { return {false, true, true, 6}; }

      void set_light(bool light);
      void reset();

    protected:
      void control(const fan::FanCall &call) override;

    private:
      StarFanStatefulRemote _remote;
    };

    class Rfm69StarFanLightSwitch : public switch_::Switch, public Parented<Rfm69StarFan>
    {
    public:
      Rfm69StarFanLightSwitch() = default;

    protected:
      void write_state(bool state) override;
    };

    class Rfm69StarFanResetButton : public button::Button, public Parented<Rfm69StarFan>
    {
    public:
      Rfm69StarFanResetButton() = default;

    protected:
      void press_action() override;
    };

  } // namespace rfm69_star_fan
} // namespace esphome
