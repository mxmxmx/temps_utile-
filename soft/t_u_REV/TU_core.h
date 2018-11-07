#ifndef TU_CORE_H_
#define TU_CORE_H_

#include <stdint.h>
#include "TU_config.h"
#include "util/util_debugpins.h"
#include "src/display.h"

namespace TU {
  namespace CORE {
  extern volatile uint32_t ticks;
  extern volatile bool app_isr_enabled;

  class ISRGuard {
  public:
    ISRGuard() {
      enabled_ = CORE::app_isr_enabled;
      CORE::app_isr_enabled = false;
      delay(1);
    }

    ~ISRGuard() {
      CORE::app_isr_enabled = enabled_;
    }

  private:
    bool enabled_;
  };
  }; // namespace CORE


  struct TickCount {
    TickCount() { }
    void Init() {
      last_ticks = 0;
    }

    uint32_t Update() {
      uint32_t now = TU::CORE::ticks;
      uint32_t ticks = now - last_ticks;
      last_ticks = now;
      return ticks;
    }

    uint32_t last_ticks;
  };

}; // namespace TU

#endif // TU_CORE_H_
