#pragma once

// Lightweight ethernet base for RP2040 W5500.
// Satisfies the interface that ESPHome's network/util.cpp expects
// when USE_ETHERNET is defined.

#include "esphome/core/component.h"
#include "esphome/components/network/ip_address.h"
#include <string>

namespace esphome
{
  namespace ethernet
  {

    class EthernetComponent : public Component
    {
    public:
      virtual bool is_connected() = 0;
      virtual network::IPAddress get_ip_address() = 0;

      virtual network::IPAddresses get_ip_addresses()
      {
        network::IPAddresses addrs{};
        addrs[0] = get_ip_address();
        return addrs;
      }

      virtual const char *get_use_address() const = 0;
      virtual void set_use_address(const std::string &use_address) = 0;
    };

    extern EthernetComponent *global_eth_component; // NOLINT

  } // namespace ethernet
} // namespace esphome
