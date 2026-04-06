#include "w5500_ethernet.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include <Arduino.h>
#include <SPI.h>
#include <W5500lwIP.h>

// The Wiznet5500lwIP object is created in setup() once we know the pin
// values, so we use a pointer here.
static Wiznet5500lwIP *eth = nullptr;

namespace esphome {

// Provide the global that network/util.cpp resolves via USE_ETHERNET.
namespace ethernet {
EthernetComponent *global_eth_component = nullptr;  // NOLINT
}  // namespace ethernet

namespace w5500_ethernet {

static const char *const TAG = "w5500_ethernet";

W5500EthernetComponent::W5500EthernetComponent() {
  ethernet::global_eth_component = this;
}

float W5500EthernetComponent::get_setup_priority() const {
  return setup_priority::ETHERNET;
}

void W5500EthernetComponent::setup() {
  ESP_LOGCONFIG(TAG, "Setting up W5500 Ethernet...");

  // Create the lwIP driver with the configured pins.
  eth = new Wiznet5500lwIP(this->cs_pin_, SPI, this->int_pin_);

  // Configure SPI pins
  SPI.setRX(this->miso_pin_);
  SPI.setCS(this->cs_pin_);
  SPI.setSCK(this->clk_pin_);
  SPI.setTX(this->mosi_pin_);

  // Hardware-reset the W5500
  pinMode(this->rst_pin_, OUTPUT);
  digitalWrite(this->rst_pin_, LOW);
  delayMicroseconds(500);
  digitalWrite(this->rst_pin_, HIGH);
  delay(200);

  // Apply static IP before begin() if configured
  if (this->has_manual_ip_) {
    auto &m = this->manual_ip_;
    IPAddress ip(m.static_ip[0], m.static_ip[1], m.static_ip[2], m.static_ip[3]);
    IPAddress gw(m.gateway[0], m.gateway[1], m.gateway[2], m.gateway[3]);
    IPAddress sn(m.subnet[0], m.subnet[1], m.subnet[2], m.subnet[3]);
    IPAddress d1(m.dns1[0], m.dns1[1], m.dns1[2], m.dns1[3]);
    IPAddress d2(m.dns2[0], m.dns2[1], m.dns2[2], m.dns2[3]);
    eth->config(ip, gw, sn, d1, d2);
    ESP_LOGI(TAG, "Static IP configured: %s", ip.toString().c_str());
  }

  // Start Ethernet (DHCP if no static IP was set)
  if (!eth->begin()) {
    ESP_LOGE(TAG, "W5500 init failed - check wiring");
    this->mark_failed();
    return;
  }

  if (this->has_manual_ip_) {
    // Static IP — no DHCP wait needed, just check link
    ESP_LOGI(TAG, "W5500 started with static IP %s",
             eth->localIP().toString().c_str());
    this->connected_ = eth->isLinked();
  } else {
    ESP_LOGI(TAG, "W5500 initialised, waiting for DHCP...");
    // Block up to 15 s for a DHCP lease so that downstream components
    // (api, web_server, http_request, sntp) find the link ready.
    uint32_t start = millis();
    while (!eth->connected() && (millis() - start) < 15000) {
      delay(100);
    }
    if (eth->connected()) {
      this->connected_ = true;
      ESP_LOGI(TAG, "DHCP lease obtained - IP %s",
               eth->localIP().toString().c_str());
    } else {
      ESP_LOGW(TAG, "DHCP timeout - will keep retrying in loop()");
    }
  }
}

void W5500EthernetComponent::loop() {
  if (eth == nullptr)
    return;

  bool now_connected = eth->connected() && eth->isLinked();

  if (now_connected != this->connected_) {
    this->connected_ = now_connected;
    if (now_connected) {
      ESP_LOGI(TAG, "Ethernet connected - IP %s",
               eth->localIP().toString().c_str());
    } else {
      ESP_LOGW(TAG, "Ethernet disconnected");
    }
  }

  // Periodic status at DEBUG level every 30 s
  uint32_t now = millis();
  if ((now - this->last_status_log_) >= 30000) {
    this->last_status_log_ = now;
    ESP_LOGD(TAG, "IP: %s | Link: %s | Heap: %lu",
             eth->localIP().toString().c_str(),
             eth->isLinked() ? "up" : "down",
             static_cast<unsigned long>(rp2040.getFreeHeap()));
  }
}

void W5500EthernetComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "W5500 Ethernet:");
  ESP_LOGCONFIG(TAG, "  Address: %s", this->use_address_.c_str());
  if (eth != nullptr) {
    ESP_LOGCONFIG(TAG, "  IP:     %s", eth->localIP().toString().c_str());
  }
  ESP_LOGCONFIG(TAG, "  SPI pins  - CS:%d  MISO:%d  MOSI:%d  CLK:%d",
                this->cs_pin_, this->miso_pin_, this->mosi_pin_,
                this->clk_pin_);
  ESP_LOGCONFIG(TAG, "  Ctrl pins - INT:%d  RST:%d", this->int_pin_,
                this->rst_pin_);
  if (this->has_manual_ip_) {
    ESP_LOGCONFIG(TAG, "  Static IP: yes");
  } else {
    ESP_LOGCONFIG(TAG, "  DHCP: yes");
  }
}

bool W5500EthernetComponent::is_connected() {
  if (eth == nullptr)
    return false;
  return eth->connected() && eth->isLinked();
}

network::IPAddress W5500EthernetComponent::get_ip_address() {
  if (eth == nullptr)
    return {};
  IPAddress ip = eth->localIP();
  return network::IPAddress(ip[0], ip[1], ip[2], ip[3]);
}

network::IPAddresses W5500EthernetComponent::get_ip_addresses() {
  network::IPAddresses addrs{};
  addrs[0] = get_ip_address();
  return addrs;
}

std::string W5500EthernetComponent::get_use_address() const {
  return this->use_address_;
}

void W5500EthernetComponent::set_use_address(const std::string &use_address) {
  this->use_address_ = use_address;
}

}  // namespace w5500_ethernet
}  // namespace esphome
