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

    /// @brief format: [timestamp] add_user -c <cur_username> -u <username> -p <password> -n <name> -m <mailAddr> -g <privilege>
    /// @return void, outputs 0 on success, -1 on failure
    void add_user(const utils::Args &args);

    /// @brief format: [timestamp] login -u <username> -p <password>
    /// @return void, outputs 0 on success, -1 on failure
    void login(const utils::Args &args);

    /// @brief format: [timestamp] logout -u <username>
    /// @return void, outputs 0 on success, -1 on failure
    void logout(const utils::Args &args);

    /// @brief format: [timestamp] query_profile -c <cur_username> -u <username>
    /// @return void, outputs a string containing the user's profile on success, -1 on failure
    void query_profile(const utils::Args &args);

    /// @brief format: [timestamp] modify_profile -c <cur_username> -u <username> (-p <password>) (-n <name>) (-m <mailAddr>) (-g <privilege>)
    /// @return void, outputs the modified user's profile on success, -1 on failure
    void modify_profile(const utils::Args &args);

    /// @brief format: [timestamp] add_train -i <trainID> -n <stationNum> -m <seatNum> -s <stations> -p <prices> -x <startTime> -t <travelTimes> -o <stopoverTimes> -d <saleDate> -y <type>
    /// @return void, outputs 0 on success, -1 on failure
    void add_train(const utils::Args &args);

    /// @brief format: [timestamp] delete_train -i <trainID>
    /// @return void, outputs 0 on success, -1 on failure
    void delete_train(const utils::Args &args);

    /// @brief format: [timestamp] release_train -i <trainID>
    /// @return void, outputs 0 on success, -1 on failure
    void release_train(const utils::Args &args);

    /// @brief format: [timestamp] query_train -i <trainID> -d <date>
    /// @return void, outputs a multi-line string containing the train details on success, -1 on failure
    void query_train(const utils::Args &args);

    /// @brief format: [timestamp] query_ticket -s <startStation> -t <endStation> -d <date> (-p <time|cost>)
    /// @return void, outputs a multi-line string containing the ticket details on success
    void query_ticket(const utils::Args &args);

    /// @brief format: [timestamp] query_transfer -s <startStation> -t <endStation> -d <date> (-p <time|cost>)
    /// @return void, outputs 0 if no matching trains, otherwise a multi-line string containing the transfer details
    void query_transfer(const utils::Args &args);

    /// @brief format: [timestamp] buy_ticket -u <username> -i <trainID> -d <date> -n <numTickets> -f <fromStation> -t <toStation> (-q <false|true>)
    /// @return void, outputs the total price on success, "queue" if added to waiting list, -1 on failure
    void buy_ticket(const utils::Args &args);

    /// @brief format: [timestamp] query_order -u <username>
    /// @return void, outputs a multi-line string containing the order details on success, -1 on failure
    void query_order(const utils::Args &args);

    /// @brief format: [timestamp] refund_ticket -u <username> (-n <orderNumber>)
    /// @return void, outputs 0 on success, -1 on failure
    void refund_ticket(const utils::Args &args);

    /// @brief format: [timestamp] clean
    /// @return void, outputs 0 on success
    void clean(const utils::Args &args);

    /// @brief format: [timestamp] exit
    /// @return void, outputs "bye"
    void exit(const utils::Args &args);
};
} // namespace business
