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
  if (date < 0) {
    return utils::FastIO::WriteFailure(); // invalid date
  }
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
  if (seat_count > train_info->seat_count) {
    return utils::FastIO::WriteFailure(); // too many tickets
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
void TicketSystem::RefundTicket(std::string_view username,
                                order_no_t order_latest_no) {
  auto user_data = GetLoggedInUser(username);
  if (user_data == logged_in_users_.end()) {
    return utils::FastIO::WriteFailure(); // user not logged in
  }
  if (order_latest_no > user_data->second.order_count) {
    return utils::FastIO::WriteFailure(); // invalid order_no
  }
  order_no_t order_no = user_data->second.order_count - order_latest_no;
  TicketInfo ticket;
  auto pos = ticket_index_.GetValue(
      {user_data->second.user_id, static_cast<order_no_t>(-order_no)}, &ticket);
  ASSERT(pos);
  if (ticket.status == TicketStatus::REFUNDED) {
    return utils::FastIO::WriteFailure(); // already refunded
  }
  if (ticket.status == TicketStatus::SUCCESS) {
    // restore vacancy
    auto train_handle = vls()->Get<TrainInfo>(ticket.train_id);
    auto train = train_handle.Get();
    auto vacancy_handle = vls()->Get<Vacancy>(
        train->GetVacancyId(ticket.date));
    auto vacancy = vacancy_handle.GetMut();
    vacancy->ReduceVacancy(
        train->station_count,
        ticket.date,
        ticket.from,
        ticket.to,
        -ticket.seat_count);
    // update pending queue
    auto pending_it = pending_queue_.LowerBound(
        {{ticket.train_id, ticket.date}, 0});
    using PendingKey = decltype(pending_it.Key());
    std::vector<PendingKey> to_delete;
    for (; pending_it != pending_queue_.End()
           && pending_it.Key().first ==
           storage::make_packed_pair(ticket.train_id, ticket.date);
           ++pending_it) {
      if (vacancy->GetVacancy(
              train->station_count, ticket.date,
              pending_it.Value().from, pending_it.Value().to) <
          pending_it.Value().seat_count) {
        continue;
      }
      TicketInfo ticket2;
      auto pos2 = ticket_index_.GetValue(
          {pending_it.Value().user_id,
           static_cast<order_no_t>(-pending_it.Value().order_no)},
          &ticket2);
      ASSERT(pos2);
      to_delete.push_back(pending_it.Key());
      if (ticket2.status != TicketStatus::PENDING) {
        continue;
      }
      ticket2.status = TicketStatus::SUCCESS;
      vacancy->ReduceVacancy(
          train->station_count,
          ticket.date,
          pending_it.Value().from,
          pending_it.Value().to,
          pending_it.Value().seat_count);
      ticket_index_.SetValue(
          {pending_it.Value().user_id,
           static_cast<order_no_t>(-pending_it.Value().order_no)},
          ticket2, pos2);
    }
    for (auto& key : to_delete) {
      bool result = pending_queue_.Remove(key);
      ASSERT(result);
    }
  }
  ticket.status = TicketStatus::REFUNDED;
  ticket_index_.SetValue(
      {user_data->second.user_id, static_cast<order_no_t>(-order_no)},
      ticket, pos);
  utils::FastIO::WriteSuccess();
}
} // namespace business