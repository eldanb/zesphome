from esphome import pins
import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.const import CONF_ID, CONF_MISO_PIN, CONF_MOSI_PIN, CONF_CLK_PIN, CONF_CS_PIN
from esphome.core import coroutine_with_priority
from esphome.components.cover import register_cover, COVER_SCHEMA

ns = cg.esphome_ns.namespace("rfm69")
Rfm69 = ns.class_("Rfm69", cg.Component)

CONF_RFM69_ID = "rfm69_id"

CONFIG_SCHEMA = cv.COMPONENT_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(Rfm69),
        cv.Required(CONF_MISO_PIN): pins.internal_gpio_output_pin_number,
        cv.Required(CONF_MOSI_PIN): pins.internal_gpio_output_pin_number,
        cv.Required(CONF_CLK_PIN): pins.internal_gpio_output_pin_number,
        cv.Required(CONF_CS_PIN): pins.internal_gpio_output_pin_number,        
    }
)


@coroutine_with_priority(80.0)
async def to_code(config):    
    rhs = Rfm69.new(config[CONF_MISO_PIN], config[CONF_MOSI_PIN], config[CONF_CLK_PIN], config[CONF_CS_PIN], -1)
    var = cg.Pvariable(config[CONF_ID], rhs)
    await cg.register_component(var, config)
