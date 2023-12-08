#pragma once

#include "esphome/core/component.h"
#include "esphome/components/cover/cover.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"
#include "esphome/core/application.h"
#include "esphome/components/rfm69/rfm69.h"
#include "somfy_remote.h"

namespace esphome
{
  namespace rfm69_somfy_cover
  {
    static const char *const TAG = "rfm69_somfy_cover";

    class Rfm69SomfyCover : public cover::Cover
    {
    public:
      Rfm69SomfyCover(rfm69::Rfm69 *rfm, unsigned long address, bool supportProg)
          : _remote(rfm, address), _supportProg(supportProg)
      {
      }

      cover::CoverTraits get_traits() override
      {
        auto traits = cover::CoverTraits();
        traits.set_is_assumed_state(true);
        return traits;
      }

      void control(const cover::CoverCall &call) override
      {
        ESP_LOGD(TAG, "cover call %s: %d", this->get_name().c_str(), call.get_position().value_or(-1));

        if (call.get_stop())
        {
          if (_supportProg)
          {
            _remote.send_command(SomfyRemote::SomfyRemoteCommandProg, 5);
          }
          else
          {
            _remote.send_command(SomfyRemote::SomfyRemoteCommandMy, 3);
          }
        }
        else if (call.get_position() == esphome::cover::COVER_OPEN)
        {
          _remote.send_command(SomfyRemote::SomfyRemoteCommandUp, 3);
        }
        else if (call.get_position() == esphome::cover::COVER_CLOSED)
        {
          _remote.send_command(SomfyRemote::SomfyRemoteCommandDown, 3);
        }
      }

      SomfyRemote _remote;
      bool _supportProg;
    };
  }
}
