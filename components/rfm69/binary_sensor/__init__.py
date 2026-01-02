import logging

import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import binary_sensor
from esphome.components.rfm69 import CONF_RFM69_ID, Rfm69
from esphome.const import (CONF_ALLOW_OTHER_USES, CONF_ID, CONF_NAME,
                           CONF_NUMBER, CONF_PIN)
from esphome.core import CORE

_LOGGER = logging.getLogger(__name__)

ns = cg.esphome_ns.namespace("rfm69_binary_sensor")
Rfm69BinarySensor = ns.class_(
    "Rfm69BinarySensor", binary_sensor.BinarySensor, cg.Component
)

CONF_ON_PACKET = "on_packet"
CONF_OFF_PACKET = "off_packet"


CONFIG_SCHEMA = (
    binary_sensor.binary_sensor_schema(Rfm69BinarySensor)
    .extend(
        {
            cv.Required(CONF_RFM69_ID): cv.use_id(Rfm69),
            cv.Required(CONF_ON_PACKET): cv.ensure_list(cv.int_),
            cv.Required(CONF_OFF_PACKET): cv.ensure_list(cv.int_)
        }
    )
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    var = await binary_sensor.new_binary_sensor(config, 
                                                await cg.get_variable(config[CONF_RFM69_ID]), 
                                                config[CONF_ON_PACKET], 
                                                config[CONF_OFF_PACKET])
    await cg.register_component(var, config)
