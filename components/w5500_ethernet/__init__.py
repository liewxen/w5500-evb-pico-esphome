import esphome.config_validation as cv
import esphome.codegen as cg
from esphome.const import (
    CONF_DOMAIN,
    CONF_ID,
    CONF_MANUAL_IP,
    CONF_STATIC_IP,
    CONF_USE_ADDRESS,
    CONF_GATEWAY,
    CONF_SUBNET,
    CONF_DNS1,
    CONF_DNS2,
)
from esphome.core import CORE, coroutine_with_priority
from esphome.components.network import IPAddress

CONFLICTS_WITH = []
DEPENDENCIES = ["rp2040"]
AUTO_LOAD = ["network", "ethernet"]

w5500_ns = cg.esphome_ns.namespace("w5500_ethernet")

ethernet_base_ns = cg.esphome_ns.namespace("ethernet")
EthernetBaseComponent = ethernet_base_ns.class_("EthernetComponent", cg.Component)

W5500EthernetComponent = w5500_ns.class_(
    "W5500EthernetComponent", EthernetBaseComponent
)
ManualIP = w5500_ns.struct("ManualIP")

# ---- Pin config keys ----
CONF_CS_PIN = "cs_pin"
CONF_MISO_PIN = "miso_pin"
CONF_MOSI_PIN = "mosi_pin"
CONF_CLK_PIN = "clk_pin"
CONF_INTERRUPT_PIN = "interrupt_pin"
CONF_RESET_PIN = "reset_pin"


def rp2040_pin(value):
    """Accept a pin number as int (17) or GPIOxx string (GPIO17)."""
    if isinstance(value, str):
        s = str(value).upper().strip()
        if s.startswith("GPIO"):
            s = s[4:]
        value = int(s)
    return cv.int_range(min=0, max=29)(value)


MANUAL_IP_SCHEMA = cv.Schema(
    {
        cv.Required(CONF_STATIC_IP): cv.ipv4address,
        cv.Required(CONF_GATEWAY): cv.ipv4address,
        cv.Required(CONF_SUBNET): cv.ipv4address,
        cv.Optional(CONF_DNS1, default="0.0.0.0"): cv.ipv4address,
        cv.Optional(CONF_DNS2, default="0.0.0.0"): cv.ipv4address,
    }
)


def _validate(config):
    if CONF_USE_ADDRESS not in config:
        if CONF_MANUAL_IP in config:
            use_address = str(config[CONF_MANUAL_IP][CONF_STATIC_IP])
        else:
            use_address = CORE.name + config[CONF_DOMAIN]
        config[CONF_USE_ADDRESS] = use_address
    return config


CONFIG_SCHEMA = cv.All(
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(W5500EthernetComponent),
            # SPI / control pins (defaults = W55RP20-EVB-Pico)
            cv.Optional(CONF_CS_PIN, default=17): rp2040_pin,
            cv.Optional(CONF_MISO_PIN, default=16): rp2040_pin,
            cv.Optional(CONF_MOSI_PIN, default=19): rp2040_pin,
            cv.Optional(CONF_CLK_PIN, default=18): rp2040_pin,
            cv.Optional(CONF_INTERRUPT_PIN, default=21): rp2040_pin,
            cv.Optional(CONF_RESET_PIN, default=20): rp2040_pin,
            # Network
            cv.Optional(CONF_MANUAL_IP): MANUAL_IP_SCHEMA,
            cv.Optional(CONF_DOMAIN, default=".local"): cv.domain_name,
            cv.Optional(CONF_USE_ADDRESS): cv.string_strict,
        }
    ).extend(cv.COMPONENT_SCHEMA),
    _validate,
)


def _ip_to_codegen(addr):
    """Convert ipaddress.IPv4Address to codegen IPAddress(a, b, c, d)."""
    return IPAddress(*list(addr.packed))


def manual_ip(config):
    return cg.StructInitializer(
        ManualIP,
        ("static_ip", _ip_to_codegen(config[CONF_STATIC_IP])),
        ("gateway", _ip_to_codegen(config[CONF_GATEWAY])),
        ("subnet", _ip_to_codegen(config[CONF_SUBNET])),
        ("dns1", _ip_to_codegen(config[CONF_DNS1])),
        ("dns2", _ip_to_codegen(config[CONF_DNS2])),
    )


@coroutine_with_priority(60.0)
async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)

    # Pins
    cg.add(var.set_cs_pin(config[CONF_CS_PIN]))
    cg.add(var.set_miso_pin(config[CONF_MISO_PIN]))
    cg.add(var.set_mosi_pin(config[CONF_MOSI_PIN]))
    cg.add(var.set_clk_pin(config[CONF_CLK_PIN]))
    cg.add(var.set_interrupt_pin(config[CONF_INTERRUPT_PIN]))
    cg.add(var.set_reset_pin(config[CONF_RESET_PIN]))

    # Network
    cg.add(var.set_use_address(config[CONF_USE_ADDRESS]))

    if CONF_MANUAL_IP in config:
        cg.add(var.set_manual_ip(manual_ip(config[CONF_MANUAL_IP])))

    # ESPHome core reads CORE.config["ethernet"]["use_address"] for the
    # device address (storage_json, mdns, etc.).  Inject it here so the
    # auto-loaded ethernet stub carries the value.
    eth_conf = CORE.config.get("ethernet")
    if eth_conf is not None:
        eth_conf[CONF_USE_ADDRESS] = config[CONF_USE_ADDRESS]
    else:
        CORE.config["ethernet"] = {CONF_USE_ADDRESS: config[CONF_USE_ADDRESS]}

    cg.add_define("USE_W5500_ETHERNET")
