#pragma once

#include "esphome/core/component.h"
#include "esphome/components/cover/cover.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include "esphome/components/rfm69/rfm69.h"
#include "pellini_remote.h"

namespace esphome
{
  namespace rfm69_pellini_cover
  {
    static const char *const TAG = "rfm69_pellini_cover";

    class Rfm69PelliniCover : public cover::Cover
    {
    public:
      Rfm69PelliniCover(rfm69::Rfm69 *rfm, unsigned long address)
          : _remote(rfm, address)
      {
      }

      cover::CoverTraits get_traits() override
      {
        auto traits = cover::CoverTraits();
        traits.set_is_assumed_state(true);
        traits.set_supports_stop(false);
        return traits;
      }

      void control(const cover::CoverCall &call) override
      {
        ESP_LOGD(TAG, "cover call %s: %d", this->get_name().c_str(), call.get_position().value_or(-1));

        if (call.get_position() == esphome::cover::COVER_OPEN)
        {
          _remote.send_command_for_duration(PelliniRemote::PelliniRemoteCommandUp, 800);
        }
        else if (call.get_position() == esphome::cover::COVER_CLOSED)
        {
          _remote.send_command_for_duration(PelliniRemote::PelliniRemoteCommandDown, 2500);
        }
      }

      PelliniRemote _remote;
      bool _supportProg;
    };
  }
}
