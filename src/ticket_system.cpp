//
// Created by zj on 5/13/2024.
//

#include "ticket_system.h"

namespace business {
void TicketSystem::BuyTicket(int timestamp, std::string_view username,
                             std::string_view train_name, date_t date,
                             std::string_view from_str,
                             std::string_view to_str, int seat_count,
                             bool agree_to_wait) {
  auto user_data = GetLoggedInUser(username);
  if (user_data == logged_in_users_.end()) {
    return utils::FastIO::WriteFailure(); // user not logged in
  }
  storage::record_id_t train_id;
  if (!train_id_index_.GetValue(storage::Hash()(train_name), &train_id)) {
    return utils::FastIO::WriteFailure(); // train not found
  }
  storage::record_id_t from, to;
  if (!station_id_index_.GetValue(storage::Hash()(from_str), &from) ||
      !station_id_index_.GetValue(storage::Hash()(to_str), &to)) {
    return utils::FastIO::WriteFailure(); // station not found
  }
  auto train_info_handle = vls()->Get<TrainInfo>(train_id);
  auto train_info = train_info_handle.Get();
  if (!train_info->IsReleased()) {
    return utils::FastIO::WriteFailure(); // train not released
  }
  int8_t from_no = train_info->GetStationNo(from);
  int8_t to_no = train_info->GetStationNo(to);
  if (from_no == -1 || to_no == -1 || from_no >= to_no) {
    return utils::FastIO::WriteFailure(); // invalid station
  }
  date_t depart_date = train_info->GetDepartDate(date, from_no);
  if (!train_info->IsOnSale(depart_date)) {
    return utils::FastIO::WriteFailure(); // train not on sale
  }
  auto vacancy_handle = vls()->Get<Vacancy>(
      train_info->GetVacancyId(depart_date));
  auto vacancy = vacancy_handle.Get();
  bool pending = false;
  if (vacancy->GetVacancy(train_info->station_count, depart_date, from_no,
                          to_no) < seat_count) {
    if (!agree_to_wait) {
      return utils::FastIO::WriteFailure(); // not enough tickets
    } else {
      pending = true;
    }
  }
  bool result = ticket_index_.Insert(
      {user_data->second.user_id,
       static_cast<order_no_t>(-user_data->second.order_count++)},
      {
          pending ? TicketStatus::PENDING : TicketStatus::SUCCESS,
          depart_date,
          from_no,
          to_no,
          train_id,
          seat_count
      }
      );
  ASSERT(result);
  if (pending) {
    result = pending_queue_.Insert(
        {{train_id, depart_date}, timestamp},
        {
            from_no,
            to_no,
            static_cast<order_no_t>(user_data->second.order_count - 1),
            seat_count,
            user_data->second.user_id
        }
        );
    ASSERT(result);
    utils::FastIO::Write("queue\n");
  } else {
    vacancy_handle->ReduceVacancy(
        train_info->station_count,
        depart_date,
        from_no,
        to_no,
        seat_count);
    int price = train_info->GetPrice(from_no, to_no) * seat_count;
    utils::FastIO::Write(price, '\n');
  }
}
void TicketSystem::QueryOrder(std::string_view username) {
  auto user_data = GetLoggedInUser(username);
  if (user_data == logged_in_users_.end()) {
    return utils::FastIO::WriteFailure(); // user not logged in
  }
  auto ticket_it = ticket_index_.LowerBound(
      {user_data->second.user_id, std::numeric_limits<order_no_t>::min()});
  /*
  查询成功：第一行输出一个整数，表示订单数量。
  接下来每一行表示一个订单，格式为 `[<STATUS>] <trainID> <FROM> <LEAVING_TIME> -> <TO> <ARRIVING_TIME> <PRICE> <NUM>`，其中 `<NUM>` 为购票数量， `<STATUS>` 表示该订单的状态，可能的值为：`success`（购票已成功）、`pending`（位于候补购票队列中）和 `refunded`（已经退票）。
  */
  utils::FastIO::Write(user_data->second.order_count, '\n');
  for (; ticket_it != ticket_index_.End()
         && ticket_it.Key().first == user_data->second.user_id; ++ticket_it) {
    auto ticket = ticket_it.Value();
    auto train_info_handle = vls()->Get<TrainInfo>(ticket.train_id);
    auto train_info = train_info_handle.Get();
    auto from_handle = vls()->Get<StationName>(
        train_info->GetStationId(ticket.from));
    auto to_handle = vls()->Get<StationName>(
        train_info->GetStationId(ticket.to));
    switch (ticket.status) {
      case TicketStatus::SUCCESS: {
        utils::FastIO::Write("[success] ");
        break;
      }
      case TicketStatus::PENDING: {
        utils::FastIO::Write("[pending] ");
        break;
      }
      case TicketStatus::REFUNDED: {
        utils::FastIO::Write("[refunded] ");
        break;
      }
    }
    utils::FastIO::Write(utils::get_field(train_info->train_name, 20), ' ',
                         utils::get_field(from_handle.Get()->name, 30), ' ',
                         utils::Parser::DateTimeString(
                             train_info->GetLeaveTime(
                                 ticket.date, ticket.from)),
                         " -> ",
                         utils::get_field(to_handle.Get()->name, 30), ' ',
                         utils::Parser::DateTimeString(
                             train_info->GetArriveTime(ticket.date, ticket.to)),
                         ' ',
                         train_info->GetPrice(ticket.from, ticket.to), ' ',
                         ticket.seat_count, '\n');
  }
}
} // namespace business