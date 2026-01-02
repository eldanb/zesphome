import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import climate_ir

CODEOWNERS = ["@eldanb"]
AUTO_LOAD = ["climate_ir"]

electra_ns = cg.esphome_ns.namespace("electra_climate")
ElectraClimate = electra_ns.class_("ElectraClimate", climate_ir.ClimateIR)


CONFIG_SCHEMA = climate_ir.climate_ir_with_receiver_schema(ElectraClimate).extend(
    {
    }
)


async def to_code(config):
    await climate_ir.new_climate_ir(config)
