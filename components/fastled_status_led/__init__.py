from esphome import pins
import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.const import CONF_ID, CONF_PIN
from esphome.core import coroutine_with_priority

status_led_ns = cg.esphome_ns.namespace("fastled_status_led")
FastLEDStatusLED = status_led_ns.class_("FastLEDStatusLED", cg.Component)

CONFIG_SCHEMA = cv.Schema(
    {
        cv.GenerateID(): cv.declare_id(FastLEDStatusLED),
        cv.Required(CONF_PIN): pins.internal_gpio_output_pin_number,
    }
).extend(cv.COMPONENT_SCHEMA)


@coroutine_with_priority(80.0)
async def to_code(config):    
    rhs = FastLEDStatusLED.new(config[CONF_PIN])
    var = cg.Pvariable(config[CONF_ID], rhs)
    await cg.register_component(var, config)    
    cg.add_library("adafruit/Adafruit Neopixel", "1.12.0")