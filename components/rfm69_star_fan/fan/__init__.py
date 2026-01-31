import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import button, fan, switch
from esphome.components.rfm69 import CONF_RFM69_ID, Rfm69
from esphome.const import (CONF_DIRECTION_OUTPUT, CONF_ID, CONF_NAME,
                           CONF_OSCILLATION_OUTPUT, CONF_OUTPUT,
                           CONF_PRESET_MODES, CONF_SPEED, CONF_SPEED_COUNT)

ns = cg.esphome_ns.namespace("rfm69_star_fan")

Rfm69StarFan = ns.class_("Rfm69StarFan", cg.Component, fan.Fan)
Rfm69StarFanLightSwitch = ns.class_("Rfm69StarFanLightSwitch", cg.Component, switch.Switch)
Rfm69StarFanResetButton = ns.class_("Rfm69StarFanResetButton", cg.Component, button.Button)

CONF_ADDRESS = "star_fan_address"
CONF_LIGHT_SWITCH = "light_switch"
CONF_RESET_BUTTON = "reset_button"

CONFIG_SCHEMA = (
    fan.fan_schema(Rfm69StarFan)
    .extend(
        {
            cv.Required(CONF_RFM69_ID): cv.use_id(Rfm69),
            cv.Required(CONF_ADDRESS): cv.int_,
            cv.Required(CONF_LIGHT_SWITCH): switch.switch_schema(
                Rfm69StarFanLightSwitch,
                icon="mdi:ceiling-light"
            ),
            cv.Required(CONF_RESET_BUTTON): button.button_schema(
                Rfm69StarFanResetButton,
                icon="mdi:restart" 
            )
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)

async def to_code(config):
    rfm69_component = await cg.get_variable(config[CONF_RFM69_ID])
    var = await fan.new_fan(config, rfm69_component, config[CONF_ADDRESS])
    await cg.register_component(var, config)

    s = await switch.new_switch(config.get(CONF_LIGHT_SWITCH))
    await cg.register_parented(s, config.get(CONF_ID))
    cg.add(var.set_light_switch(s))
    
    b = await button.new_button(config.get(CONF_RESET_BUTTON))
    await cg.register_parented(b, config.get(CONF_ID))
    