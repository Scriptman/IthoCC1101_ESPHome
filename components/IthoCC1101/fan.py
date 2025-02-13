import esphome.codegen as cg
import esphome.config_validation as cv
from esphome import pins
from esphome.components import fan
from esphome.const import (
    CONF_ID,
    CONF_IRQ_PIN
)

DEPENDENCIES = ['esp32']

itho_ecofan_rft_ns = cg.esphome_ns.namespace('itho_ecofan_rft')
IthoEcoFanRFTFan = itho_ecofan_rft_ns.class_('IthoEcoFanRFT', fan.Fan, cg.Component)

CONFIG_SCHEMA = fan.FAN_SCHEMA.extend({
    cv.GenerateID(): cv.declare_id(IthoEcoFanRFTFan),
    cv.required(CONF_IRQ_PIN): pins.gpio_input_pin_schema,

}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    yield cg.register_component(var, config)
    yield fan.register_fan(var, config)

    # Pass config values to c++
    pin = await cg.gpio_pin_expression(config[CONF_IRQ_PIN])
    cg.add(var.set_irq_pin(pin))

    # Add any additional setup code here
    # For example, setting up pins or initializing variables