#pragma once

#include "esphome/components/ethernet/ethernet_component.h"
#include "esphome/components/network/ip_address.h"

namespace esphome
{
  namespace w5500_ethernet
  {

    struct ManualIP
    {
      network::IPAddress static_ip;
      network::IPAddress gateway;
      network::IPAddress subnet;
      network::IPAddress dns1;
      network::IPAddress dns2;
    };

    class W5500EthernetComponent : public ethernet::EthernetComponent
    {
    public:
      W5500EthernetComponent();
      void setup() override;
      void loop() override;
      void dump_config() override;
      float get_setup_priority() const override;

      bool is_connected() override;
      network::IPAddress get_ip_address() override;
      network::IPAddresses get_ip_addresses() override;
      const char *get_use_address() const override;
      void set_use_address(const std::string &use_address) override;

      // Pin setters
      void set_cs_pin(uint8_t pin) { this->cs_pin_ = pin; }
      void set_miso_pin(uint8_t pin) { this->miso_pin_ = pin; }
      void set_mosi_pin(uint8_t pin) { this->mosi_pin_ = pin; }
      void set_clk_pin(uint8_t pin) { this->clk_pin_ = pin; }
      void set_interrupt_pin(uint8_t pin) { this->int_pin_ = pin; }
      void set_reset_pin(uint8_t pin) { this->rst_pin_ = pin; }

      // Static IP
      void set_manual_ip(const ManualIP &manual_ip)
      {
        this->manual_ip_ = manual_ip;
        this->has_manual_ip_ = true;
      }

    protected:
      std::string use_address_;
      bool connected_{false};
      uint32_t last_status_log_{0};

      // Pins (defaults = W55RP20-EVB-Pico)
      uint8_t cs_pin_{17};
      uint8_t miso_pin_{16};
      uint8_t mosi_pin_{19};
      uint8_t clk_pin_{18};
      uint8_t int_pin_{21};
      uint8_t rst_pin_{20};

      // Static IP
      bool has_manual_ip_{false};
      ManualIP manual_ip_{};
    };

  } // namespace w5500_ethernet
} // namespace esphome
