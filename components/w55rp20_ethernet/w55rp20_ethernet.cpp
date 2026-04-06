#include "w55rp20_ethernet.h"
#include "esphome/core/log.h"
#include "esphome/core/hal.h"
#include <Arduino.h>
#include <SPI.h>
#include <W55RP20lwIP.h>

// The Wiznet55rp20lwIP object is created in setup() once we know the pin
// values, so we use a pointer here.
static Wiznet55rp20lwIP *eth = nullptr;

namespace esphome
{

  // Provide the global that network/util.cpp resolves via USE_ETHERNET.
  namespace ethernet
  {
    EthernetComponent *global_eth_component = nullptr; // NOLINT
  } // namespace ethernet

  namespace w55rp20_ethernet
  {

    static const char *const TAG = "w55rp20_ethernet";

    W55RP20EthernetComponent::W55RP20EthernetComponent()
    {
      ethernet::global_eth_component = this;
    }

    float W55RP20EthernetComponent::get_setup_priority() const
    {
      return setup_priority::ETHERNET;
    }

    void W55RP20EthernetComponent::setup()
    {
      ESP_LOGCONFIG(TAG, "Setting up W55RP20 Ethernet...");

      // Create the lwIP driver with the configured pins.
      eth = new Wiznet55rp20lwIP(this->cs_pin_, SPI, this->int_pin_);

      // Configure SPI pins
      SPI.setRX(this->miso_pin_);
      SPI.setCS(this->cs_pin_);
      SPI.setSCK(this->clk_pin_);
      SPI.setTX(this->mosi_pin_);

      // Hardware-reset the W55RP20
      pinMode(this->rst_pin_, OUTPUT);
      digitalWrite(this->rst_pin_, LOW);
      delayMicroseconds(500);
      digitalWrite(this->rst_pin_, HIGH);
      delay(200);

      // Apply static IP before begin() if configured
      if (this->has_manual_ip_)
      {
        auto &m = this->manual_ip_;
        // network::IPAddress has operator arduino_ns::IPAddress() built in
        IPAddress ip = static_cast<IPAddress>(m.static_ip);
        IPAddress gw = static_cast<IPAddress>(m.gateway);
        IPAddress sn = static_cast<IPAddress>(m.subnet);
        IPAddress d1 = static_cast<IPAddress>(m.dns1);
        IPAddress d2 = static_cast<IPAddress>(m.dns2);
        eth->config(ip, gw, sn, d1, d2);
        ESP_LOGI(TAG, "Static IP configured: %s", ip.toString().c_str());
      }

      // Start Ethernet (DHCP if no static IP was set)
      if (!eth->begin())
      {
        ESP_LOGE(TAG, "W55RP20 init failed - check wiring");
        this->mark_failed();
        return;
      }

      if (this->has_manual_ip_)
      {
        // Static IP -- no DHCP wait needed, just check link
        ESP_LOGI(TAG, "W55RP20 started with static IP %s",
                 eth->localIP().toString().c_str());
        this->connected_ = eth->isLinked();
      }
      else
      {
        ESP_LOGI(TAG, "W55RP20 initialised, waiting for DHCP...");
        // Block up to 15 s for a DHCP lease so that downstream components
        // (api, web_server, http_request, sntp) find the link ready.
        uint32_t start = millis();
        while (!eth->connected() && (millis() - start) < 15000)
        {
          delay(100);
        }
        if (eth->connected())
        {
          this->connected_ = true;
          ESP_LOGI(TAG, "DHCP lease obtained - IP %s",
                   eth->localIP().toString().c_str());
        }
        else
        {
          ESP_LOGW(TAG, "DHCP timeout - will keep retrying in loop()");
        }
      }
    }

    void W55RP20EthernetComponent::loop()
    {
      if (eth == nullptr)
        return;

      bool now_connected = eth->connected();

      if (now_connected != this->connected_)
      {
        this->connected_ = now_connected;
        if (now_connected)
        {
          ESP_LOGI(TAG, "Ethernet connected - IP %s",
                   eth->localIP().toString().c_str());
        }
        else
        {
          ESP_LOGW(TAG, "Ethernet disconnected");
        }
      }

      // Periodic status at DEBUG level every 30 s
      uint32_t now = millis();
      if ((now - this->last_status_log_) >= 30000)
      {
        this->last_status_log_ = now;
        ESP_LOGD(TAG, "IP: %s | connected()=%d | isLinked()=%d | Heap: %lu",
                 eth->localIP().toString().c_str(),
                 static_cast<int>(eth->connected()),
                 static_cast<int>(eth->isLinked()),
                 static_cast<unsigned long>(rp2040.getFreeHeap()));
      }
    }

    void W55RP20EthernetComponent::dump_config()
    {
      ESP_LOGCONFIG(TAG, "W55RP20 Ethernet:");
      ESP_LOGCONFIG(TAG, "  Address: %s", this->use_address_.c_str());
      if (eth != nullptr)
      {
        ESP_LOGCONFIG(TAG, "  IP:     %s", eth->localIP().toString().c_str());
      }
      ESP_LOGCONFIG(TAG, "  SPI pins  - CS:%d  MISO:%d  MOSI:%d  CLK:%d",
                    this->cs_pin_, this->miso_pin_, this->mosi_pin_,
                    this->clk_pin_);
      ESP_LOGCONFIG(TAG, "  Ctrl pins - INT:%d  RST:%d", this->int_pin_,
                    this->rst_pin_);
      if (this->has_manual_ip_)
      {
        ESP_LOGCONFIG(TAG, "  Static IP: yes");
      }
      else
      {
        ESP_LOGCONFIG(TAG, "  DHCP: yes");
      }
    }

    bool W55RP20EthernetComponent::is_connected()
    {
      if (eth == nullptr)
        return false;
      // Use connected() only (checks IP is set).
      // isLinked() reads PHY register which may not report correctly
      // on all W55RP20 boards despite the physical link being up.
      return eth->connected();
    }

    network::IPAddress W55RP20EthernetComponent::get_ip_address()
    {
      if (eth == nullptr)
        return {};
      IPAddress ip = eth->localIP();
      return network::IPAddress(static_cast<uint8_t>(ip[0]), static_cast<uint8_t>(ip[1]),
                                static_cast<uint8_t>(ip[2]), static_cast<uint8_t>(ip[3]));
    }

    network::IPAddresses W55RP20EthernetComponent::get_ip_addresses()
    {
      network::IPAddresses addrs{};
      addrs[0] = get_ip_address();
      return addrs;
    }

    const char *W55RP20EthernetComponent::get_use_address() const
    {
      return this->use_address_.c_str();
    }

    void W55RP20EthernetComponent::set_use_address(const std::string &use_address)
    {
      this->use_address_ = use_address;
    }

  } // namespace w55rp20_ethernet
} // namespace esphome
