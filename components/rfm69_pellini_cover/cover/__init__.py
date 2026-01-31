import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components.cover import cover_schema, register_cover
from esphome.components.rfm69 import CONF_RFM69_ID, Rfm69
from esphome.const import (CONF_CLK_PIN, CONF_CS_PIN, CONF_ID, CONF_MISO_PIN,
                           CONF_MOSI_PIN)
from esphome.core import coroutine_with_priority

ns = cg.esphome_ns.namespace("rfm69_pellini_cover")

Rfm69PelliniCover = ns.class_("Rfm69PelliniCover", cg.Component)

CONF_ADDRESS = "pellini_address"

CONFIG_SCHEMA = cover_schema(Rfm69PelliniCover).extend(
    {
        cv.GenerateID(): cv.declare_id(Rfm69PelliniCover),
        cv.Required(CONF_RFM69_ID): cv.use_id(Rfm69),
        cv.Required(CONF_ADDRESS): cv.int_
    }
)


@coroutine_with_priority(80.0)
async def to_code(config):    
    rfm69_component = await cg.get_variable(config[CONF_RFM69_ID])
    rhs = Rfm69PelliniCover.new(rfm69_component, config[CONF_ADDRESS])
    var = cg.Pvariable(config[CONF_ID], rhs)
    await register_cover(var, config)
