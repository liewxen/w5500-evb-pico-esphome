#pragma once

#include "esphome/components/ethernet/ethernet_component.h"

namespace esphome {
namespace w5500_ethernet {

class W5500EthernetComponent : public ethernet::EthernetComponent {
 public:
  W5500EthernetComponent();
  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override;

  bool is_connected() override;
  network::IPAddress get_ip_address() override;
  network::IPAddresses get_ip_addresses() override;
  std::string get_use_address() const override;
  void set_use_address(const std::string &use_address) override;

 protected:
  std::string use_address_;
  bool connected_{false};
  uint32_t last_status_log_{0};
};

}  // namespace w5500_ethernet
}  // namespace esphome
