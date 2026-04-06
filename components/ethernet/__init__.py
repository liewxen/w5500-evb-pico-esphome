"""Ethernet stub component for RP2040 W5500.

Overrides ESPHome's built-in ESP32 ethernet component so that
network/util.cpp can find ethernet::global_eth_component when
USE_ETHERNET is defined.  The actual implementation lives in
the w5500_ethernet component.
"""
import esphome.config_validation as cv
import esphome.codegen as cg

CODEOWNERS = []

CONFIG_SCHEMA = cv.Schema({})


async def to_code(config):
    cg.add_define("USE_ETHERNET")
