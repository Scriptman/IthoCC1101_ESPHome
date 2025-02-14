import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import fan
from esphome.cpp_helpers import gpio_pin_expression

from esphome.const import (
    CONF_ID,
    CONF_IRQ_PIN,
    CONF_SPEED_COUNT,
    CONF_RESTORE_MODE
)

MAP_OFF_TO_ZERO = "map_off_to_zero"

# DEPENDENCIES = ['esp32']

# init cpp class
ithoecorft_ns = cg.esphome_ns.namespace('IthoEcoRFTFan')
IthoEcoRFTFan = ithoecorft_ns.class_('IthoEcoRFT', cg.PollingComponent, fan.Fan)

FAN_RESTORE_MODE_OPTIONS = {
    "ALWAYS_ON": fan.FanRestoreMode.ALWAYS_ON,
    "ALWAYS_OFF": fan.FanRestoreMode.ALWAYS_OFF
}

CONFIG_SCHEMA = fan.FAN_SCHEMA.extend({
    cv.GenerateID(): cv.declare_id(IthoEcoRFTFan),
    cv.Required(CONF_IRQ_PIN): pins.gpio_input_pin_schema,
    cv.Optional(CONF_SPEED_COUNT, default=3): cv.int_range(min=1, max=100),
    cv.Optional(CONF_RESTORE_MODE, default="ALWAYS_ON"): cv.enum(FAN_RESTORE_MODE_OPTIONS)
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID], config[CONF_SPEED_COUNT])
    yield cg.register_component(var, config)
    yield fan.register_fan(var, config)

    # Pass config values to c++
    pin = await cg.gpio_pin_expression(config[CONF_IRQ_PIN])
    cg.add(var.set_irq_pin(pin))

    # Add any additional setup code here
    # For example, setting up pins or initializing variables
    return var