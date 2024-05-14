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
#define ROUTE(cmd) if (command == #cmd) { cmd(args); continue; }
    ROUTE(add_user);
    ROUTE(login);
#undef ROUTE
  }
}
void TicketSystemCLI::add_user(const utils::Args& args) {
  auto privilege = args.GetFlag('g').empty() ? 10 : args.GetFlag('g')[0] - 'a';
  ticket_system_->AddUser(args.GetFlag('c'), args.GetFlag('u'), args.GetFlag('p'),
                          args.GetFlag('n'), args.GetFlag('m'),
                          privilege);
}
void TicketSystemCLI::login(const utils::Args& args) {
  ticket_system_->Login(args.GetFlag('u'), args.GetFlag('p'));
}
} // namespace business
