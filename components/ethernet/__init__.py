"""Ethernet stub component for RP2040 W5500.

Overrides ESPHome's built-in ESP32 ethernet component so that
network/util.cpp can find ethernet::global_eth_component when
USE_ETHERNET is defined.  The actual implementation lives in
the w5500_ethernet component.

ESPHome's core reads CORE.config["ethernet"]["use_address"] to
determine the device address, so this stub must carry that key.
The w5500_ethernet component's _validate() copies it here.
"""
import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.const import CONF_USE_ADDRESS

CODEOWNERS = []

CONFIG_SCHEMA = cv.Schema(
    {
        cv.Optional(CONF_USE_ADDRESS): cv.string_strict,
    }
)


async def to_code(config):
    cg.add_define("USE_ETHERNET")
