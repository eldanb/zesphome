import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import automation, pins
from esphome.const import (CONF_CLK_PIN, CONF_CS_PIN, CONF_DELAY,
                           CONF_DEVICE_CLASS, CONF_ENTITY_CATEGORY,
                           CONF_FILTERS, CONF_ICON, CONF_ID,
                           CONF_INVALID_COOLDOWN, CONF_INVERTED,
                           CONF_MAX_LENGTH, CONF_MIN_LENGTH, CONF_MISO_PIN,
                           CONF_MOSI_PIN, CONF_MQTT_ID, CONF_ON_CLICK,
                           CONF_ON_DOUBLE_CLICK, CONF_ON_MULTI_CLICK,
                           CONF_ON_PRESS, CONF_ON_RELEASE, CONF_ON_STATE,
                           CONF_PUBLISH_INITIAL_STATE, CONF_STATE, CONF_TIMING,
                           CONF_TRIGGER_ID, CONF_WEB_SERVER,
                           DEVICE_CLASS_BATTERY, DEVICE_CLASS_BATTERY_CHARGING,
                           DEVICE_CLASS_CARBON_MONOXIDE, DEVICE_CLASS_COLD,
                           DEVICE_CLASS_CONNECTIVITY, DEVICE_CLASS_DOOR,
                           DEVICE_CLASS_EMPTY, DEVICE_CLASS_GARAGE_DOOR,
                           DEVICE_CLASS_GAS, DEVICE_CLASS_HEAT,
                           DEVICE_CLASS_LIGHT, DEVICE_CLASS_LOCK,
                           DEVICE_CLASS_MOISTURE, DEVICE_CLASS_MOTION,
                           DEVICE_CLASS_MOVING, DEVICE_CLASS_OCCUPANCY,
                           DEVICE_CLASS_OPENING, DEVICE_CLASS_PLUG,
                           DEVICE_CLASS_POWER, DEVICE_CLASS_PRESENCE,
                           DEVICE_CLASS_PROBLEM, DEVICE_CLASS_RUNNING,
                           DEVICE_CLASS_SAFETY, DEVICE_CLASS_SMOKE,
                           DEVICE_CLASS_SOUND, DEVICE_CLASS_TAMPER,
                           DEVICE_CLASS_UPDATE, DEVICE_CLASS_VIBRATION,
                           DEVICE_CLASS_WINDOW)
from esphome.core import coroutine_with_priority

CONF_RFM69_ID = "rfm69_id"
CONF_RFM69_DIO2 = "dio2_pin"
CONF_ON_TRANSMITTER_BUSY_CHANGE = "on_transmitter_busy_change"

ns = cg.esphome_ns.namespace("rfm69")
Rfm69 = ns.class_("Rfm69", cg.Component)
TransmitterBusyChangeTrigger = ns.class_("TransmitterBusyChangeTrigger", automation.Trigger.template())

CONFIG_SCHEMA = cv.COMPONENT_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(Rfm69),
        cv.Required(CONF_MISO_PIN): pins.internal_gpio_output_pin_number,
        cv.Required(CONF_MOSI_PIN): pins.internal_gpio_output_pin_number,
        cv.Required(CONF_CLK_PIN): pins.internal_gpio_output_pin_number,
        cv.Required(CONF_CS_PIN): pins.internal_gpio_output_pin_number,        
        cv.Optional(CONF_RFM69_DIO2): pins.internal_gpio_output_pin_number,
        cv.Optional(CONF_ON_TRANSMITTER_BUSY_CHANGE): cv.All(
                automation.validate_automation(
                    {
                        cv.GenerateID(CONF_TRIGGER_ID): cv.declare_id(TransmitterBusyChangeTrigger),
                    }
                )
            ),
    }
)


@coroutine_with_priority(80.0)
async def to_code(config):    
    rhs = Rfm69.new(config[CONF_MISO_PIN], config[CONF_MOSI_PIN], config[CONF_CLK_PIN], config[CONF_CS_PIN],
                    config.get(CONF_RFM69_DIO2, -1))
    var = cg.Pvariable(config[CONF_ID], rhs)
    await cg.register_component(var, config)

    for conf in config.get(CONF_ON_TRANSMITTER_BUSY_CHANGE, []):
        trigger = cg.new_Pvariable(conf[CONF_TRIGGER_ID], var)
        await automation.build_automation(trigger, [], conf)
