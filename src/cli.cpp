//
// Created by zj on 5/13/2024.
//

#include "cli.h"

#include <iostream>

namespace business {
TicketSystemCLI::TicketSystemCLI(bool force_reset) {
  bool reset = force_reset;
  if (!reset) {
    std::ifstream file(storage::DB_FILE_NAME);
    reset = !file.good();
  }
  ticket_system_ = std::make_unique<TicketSystem>(storage::DB_FILE_NAME, reset);
}
void TicketSystemCLI::run() {
  std::string line;
  while (std::getline(std::cin, line)) {
    auto [command, args] = utils::Parser::Read(line);
#define ROUTE(cmd) if (command == #cmd) { WriteTimestamp(args); cmd(args); WriteDone(); continue; }
    ROUTE(add_user);
    ROUTE(login);
    ROUTE(logout);
    ROUTE(query_profile);
    ROUTE(modify_profile);
    ROUTE(add_train);
    ROUTE(delete_train);
    ROUTE(release_train);
    ROUTE(query_train);
    ROUTE(query_ticket);
    ROUTE(query_transfer);
    ROUTE(buy_ticket);
    ROUTE(query_order);
    ROUTE(refund_ticket);
    ROUTE(clean);
#undef ROUTE
    if (command == "exit") {
      WriteTimestamp(args);
      exit(args);
      WriteDone();
      break;
    }
    ASSERT(false); // No such command
  }
}
void TicketSystemCLI::add_user(const utils::Args& args) {
  int8_t privilege = args.GetFlag('g').empty()
                       ? -1
                       : utils::stoi(args.GetFlag('g'));
  ticket_system_->AddUser(args.GetFlag('c'), args.GetFlag('u'),
                          args.GetFlag('p'),
                          args.GetFlag('n'), args.GetFlag('m'),
                          privilege);
}
void TicketSystemCLI::login(const utils::Args& args) {
  ticket_system_->Login(args.GetFlag('u'), args.GetFlag('p'));
}
void TicketSystemCLI::logout(const utils::Args& args) {
  ticket_system_->Logout(args.GetFlag('u'));
}
void TicketSystemCLI::query_profile(const utils::Args& args) {
  ticket_system_->QueryProfile(args.GetFlag('c'), args.GetFlag('u'));
}
void TicketSystemCLI::modify_profile(const utils::Args& args) {
  int8_t privilege = args.GetFlag('g').empty()
                       ? -1
                       : utils::stoi(args.GetFlag('g'));
  ticket_system_->ModifyProfile(args.GetFlag('c'), args.GetFlag('u'),
                                args.GetFlag('p'), args.GetFlag('n'),
                                args.GetFlag('m'), privilege);
}
void TicketSystemCLI::add_train(const utils::Args& args) {
  ticket_system_->AddTrain(args.GetFlag('i'),
                           utils::stoi(args.GetFlag('n')),
                           utils::stoi(args.GetFlag('m')),
                           utils::Parser::ParseTime(args.GetFlag('x')),
                           args.GetFlag('y')[0],
                           utils::Parser::DelimitedStrIterator(
                               args.GetFlag('s')),
                           utils::Parser::DelimitedStrIterator(
                               args.GetFlag('p')),
                           utils::Parser::DelimitedStrIterator(
                               args.GetFlag('t')),
                           utils::Parser::DelimitedStrIterator(
                               args.GetFlag('o')),
                           utils::Parser::DelimitedStrIterator(
                               args.GetFlag('d')));
}
void TicketSystemCLI::delete_train(const utils::Args& args) {
  ticket_system_->DeleteTrain(args.GetFlag('i'));
}
void TicketSystemCLI::release_train(const utils::Args& args) {
  ticket_system_->ReleaseTrain(args.GetFlag('i'));
}
void TicketSystemCLI::query_train(const utils::Args& args) {
  ticket_system_->QueryTrain(args.GetFlag('i'),
                             utils::Parser::ParseDate(args.GetFlag('d')));
}
void TicketSystemCLI::query_ticket(const utils::Args& args) {
  ticket_system_->QueryTicket(args.GetFlag('s'), args.GetFlag('t'),
                              utils::Parser::ParseDate(args.GetFlag('d')),
                              args.GetFlag('p'));
}
void TicketSystemCLI::query_transfer(const utils::Args& args) {
  ticket_system_->QueryTransfer(args.GetFlag('s'), args.GetFlag('t'),
                                utils::Parser::ParseDate(args.GetFlag('d')),
                                args.GetFlag('p'));
}
void TicketSystemCLI::buy_ticket(const utils::Args& args) {
  bool agree_to_wait = args.GetFlag('q') == "true";
  ticket_system_->BuyTicket(args.GetTimestamp(), args.GetFlag('u'),
                            args.GetFlag('i'),
                            utils::Parser::ParseDate(args.GetFlag('d')),
                            args.GetFlag('f'), args.GetFlag('t'),
                            utils::stoi(args.GetFlag('n')),
                            agree_to_wait);
}
void TicketSystemCLI::query_order(const utils::Args& args) {
  ticket_system_->QueryOrder(args.GetFlag('u'));
}
void TicketSystemCLI::refund_ticket(const utils::Args& args) {
  // return utils::FastIO::Write("0\n");
  order_no_t order_no = args.GetFlag('n').empty()
                          ? 0
                          : utils::stoi(args.GetFlag('n'));
  ticket_system_->RefundTicket(args.GetFlag('u'), order_no);
}
void TicketSystemCLI::clean(const utils::Args& args) {
  ticket_system_ = std::make_unique<TicketSystem>(storage::DB_FILE_NAME, true);
  utils::FastIO::WriteSuccess();
}
void TicketSystemCLI::exit(const utils::Args& args) {
  utils::FastIO::Write("bye\n");
}
} // namespace business