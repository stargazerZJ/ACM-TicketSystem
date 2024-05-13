//
// Created by zj on 5/13/2024.
//

#pragma once
#include <memory>

#include "parser.h"
#include "ticket_system.h"

namespace business {
class TicketSystemCLI {
  public:
    void run();

  private:
    // using Func = void (TicketSystemCLI::*)(const utils::Args &args);
  std::unique_ptr<TicketSystem> ticket_system_;
};
} // namespace business
