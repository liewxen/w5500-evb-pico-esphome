#include "w5500_ethernet.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include <Arduino.h>
#include <SPI.h>
#include <W5500lwIP.h>

// W55RP20-EVB-Pico pin definitions
static constexpr uint8_t W5500_CS   = 17;
static constexpr uint8_t W5500_MISO = 16;
static constexpr uint8_t W5500_MOSI = 19;
static constexpr uint8_t W5500_CLK  = 18;
static constexpr uint8_t W5500_INT  = 21;
static constexpr uint8_t W5500_RST  = 20;

static Wiznet5500lwIP eth(W5500_CS, SPI, W5500_INT);

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

  // Configure SPI pins for W55RP20-EVB-Pico
  SPI.setRX(W5500_MISO);
  SPI.setCS(W5500_CS);
  SPI.setSCK(W5500_CLK);
  SPI.setTX(W5500_MOSI);

  // Hardware-reset the W5500
  pinMode(W5500_RST, OUTPUT);
  digitalWrite(W5500_RST, LOW);
  delayMicroseconds(500);
  digitalWrite(W5500_RST, HIGH);
  delay(200);

  // Start Ethernet (DHCP)
  if (!eth.begin()) {
    ESP_LOGE(TAG, "W5500 init failed - check wiring");
    this->mark_failed();
    return;
  }

  ESP_LOGI(TAG, "W5500 initialised, waiting for DHCP...");

  // Block up to 15 s for a DHCP lease so that downstream components
  // (api, web_server, http_request, sntp) find the link ready.
  uint32_t start = millis();
  while (!eth.connected() && (millis() - start) < 15000) {
    delay(100);
  }

  if (eth.connected()) {
    this->connected_ = true;
    ESP_LOGI(TAG, "DHCP lease obtained - IP %s", eth.localIP().toString().c_str());
  } else {
    ESP_LOGW(TAG, "DHCP timeout - will keep retrying in loop()");
  }
}

void W5500EthernetComponent::loop() {
  bool now_connected = eth.connected() && eth.isLinked();

  if (now_connected != this->connected_) {
    this->connected_ = now_connected;
    if (now_connected) {
      ESP_LOGI(TAG, "Ethernet connected - IP %s", eth.localIP().toString().c_str());
    } else {
      ESP_LOGW(TAG, "Ethernet disconnected");
    }
  }

  // Periodic status at DEBUG level every 30 s
  uint32_t now = millis();
  if ((now - this->last_status_log_) >= 30000) {
    this->last_status_log_ = now;
    ESP_LOGD(TAG, "IP: %s | Link: %s | Heap: %lu",
             eth.localIP().toString().c_str(),
             eth.isLinked() ? "up" : "down",
             static_cast<unsigned long>(rp2040.getFreeHeap()));
  }
}

void W5500EthernetComponent::dump_config() {
  ESP_LOGCONFIG(TAG, "W5500 Ethernet:");
  ESP_LOGCONFIG(TAG, "  Address: %s", this->use_address_.c_str());
  ESP_LOGCONFIG(TAG, "  IP:     %s", eth.localIP().toString().c_str());
  ESP_LOGCONFIG(TAG, "  SPI pins  - CS:%d  MISO:%d  MOSI:%d  CLK:%d",
                W5500_CS, W5500_MISO, W5500_MOSI, W5500_CLK);
  ESP_LOGCONFIG(TAG, "  Ctrl pins - INT:%d  RST:%d", W5500_INT, W5500_RST);
}

bool W5500EthernetComponent::is_connected() {
  return eth.connected() && eth.isLinked();
}

network::IPAddress W5500EthernetComponent::get_ip_address() {
  IPAddress ip = eth.localIP();
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
