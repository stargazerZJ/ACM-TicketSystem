//
// Created by zj on 5/13/2024.
//

#include "cli.h"

#include <iostream>

namespace business {
void TicketSystemCLI::run() {
  std::string line;
  while (std::getline(std::cin, line)) {
    auto [command, args] = utils::Parser::Read(line);
    // run command
  }
}
} // namespace business
