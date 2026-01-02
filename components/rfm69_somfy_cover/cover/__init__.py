import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components.cover import cover_schema, register_cover
from esphome.components.rfm69 import CONF_RFM69_ID, Rfm69
from esphome.const import (CONF_CLK_PIN, CONF_CS_PIN, CONF_ID, CONF_MISO_PIN,
                           CONF_MOSI_PIN)
from esphome.core import coroutine_with_priority

ns = cg.esphome_ns.namespace("rfm69_somfy_cover")

RFM69SomfyCover = ns.class_("Rfm69SomfyCover", cg.Component)

CONF_SUPPORT_PROG = "enable_prog"
CONF_ADDRESS = "somfy_address"


CONFIG_SCHEMA = cover_schema(RFM69SomfyCover).extend(
    {
        cv.GenerateID(): cv.declare_id(RFM69SomfyCover),
        cv.Required(CONF_RFM69_ID): cv.use_id(Rfm69),
        cv.Required(CONF_SUPPORT_PROG): cv.boolean,
        cv.Required(CONF_ADDRESS): cv.int_
    }
)


@coroutine_with_priority(80.0)
async def to_code(config):    
    rfm69_compponent = await cg.get_variable(config[CONF_RFM69_ID])
    rhs = RFM69SomfyCover.new(rfm69_compponent, config[CONF_ADDRESS], config[CONF_SUPPORT_PROG])
    var = cg.Pvariable(config[CONF_ID], rhs)
    await register_cover(var, config)
