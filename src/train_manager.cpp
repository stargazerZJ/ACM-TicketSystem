//
// Created by zj on 5/13/2024.
//

#include "train_manager.h"

#include <hash.h>

namespace business {
storage::record_id_t TrainInfo::GetStationId(int station_no) const {
  if (station_no == 0) return depart_station;
  ASSERT(station_no < station_count);
  return station[station_no - 1].station_id;
}
int TrainInfo::GetStationNo(storage::record_id_t station_id) const {
  if (station_id == depart_station) return 0;
  for (int i = 0; i < station_count - 1; ++i) {
    if (station[i].station_id == station_id) return i + 1;
  }
  return -1;
}
abs_time_t TrainInfo::GetArriveTime(date_t date, int station_no) const {
  if (station_no == 0) return -1;
  ASSERT(station_no < station_count);
  return date * 1440
         + depart_time + station[station_no - 1].arrive_time;
}
abs_time_t TrainInfo::GetLeaveTime(date_t date, int station_no) const {
  if (station_no == 0) return date * 1440 + depart_time;
  ASSERT(station_no < station_count);
  if (station_no == station_count - 1) return -1;
  return date * 1440
         + depart_time + station[station_no].leave_time;
}
time_t TrainInfo::GetTravelTime(int from, int to) const {
  ASSERT(from < station_count && to < station_count && from < to);
  if (from == 0) return station[to - 1].arrive_time;
  return station[to - 1].arrive_time - station[from - 1].leave_time;
}
int TrainInfo::GetPrice(int station_no) const {
  if (station_no == 0) return 0;
  ASSERT(station_no < station_count);
  return station[station_no - 1].price;
}
int TrainInfo::GetPrice(int from, int to) const {
  ASSERT(from < station_count && to < station_count && from < to);
  return GetPrice(to) - GetPrice(from);
}
date_t TrainInfo::
GetDepartDate(date_t leaving_date, int leaving_station) const {
  ASSERT(leaving_station < station_count - 1);
  // the last station is not included
  if (leaving_station == 0) return leaving_date;
  return leaving_date - (depart_time + station[leaving_station - 1].leave_time)
         / 1440;
}
int Vacancy::GetVacancy(int8_t station_count, date_t date,
                        int station_no) const {
  ASSERT(station_no < station_count - 1); // the last station is not included
  return vacancy[(date % DATE_BATCH_SIZE) * (station_count - 1) + station_no];
}
const int* Vacancy::GetVacancy(int8_t station_count, date_t date) const {
  return vacancy + (date % DATE_BATCH_SIZE) * (station_count - 1);
}
int* Vacancy::GetVacancy(int8_t station_count, date_t date) {
  return vacancy + (date % DATE_BATCH_SIZE) * (station_count - 1);
}
int Vacancy::GetVacancy(int8_t station_count, date_t date, int from,
                        int to) const {
  ASSERT(from < station_count && to < station_count && from < to);
  // the last station is not included
  // the vacancy of a range [from, to] is the minimum of the vacancies of all the stations in the range
  // TODO(opt): use SIMD
  int min_vacancy = std::numeric_limits<int>::max();
  auto vacancy = this->GetVacancy(station_count, date);
  for (int i = from; i < to; ++i) {
    min_vacancy = std::min(min_vacancy, vacancy[i]);
  }
  return min_vacancy;
}
void Vacancy::ReduceVacancy(int8_t station_count, date_t date, int from, int to,
                            int num) {
  ASSERT(from < station_count && to < station_count && from < to);
  // the last station is not included
  auto vacancy = this->GetVacancy(station_count, date);
  for (int i = from; i < to; ++i) {
    vacancy[i] -= num;
  }
}
void TrainManager::AddTrain(std::string_view train_name, int8_t station_count,
                            int seat_count, time_t depart_time, char type,
                            utils::Parser::DelimitedStrIterator stations,
                            utils::Parser::DelimitedStrIterator prices,
                            utils::Parser::DelimitedStrIterator travel_times,
                            utils::Parser::DelimitedStrIterator stopover_times,
                            utils::Parser::DelimitedStrIterator sell_dates) {
  // 1. Allocate space for the train
  storage::VarLengthStore::Handle<TrainInfo> train_info_handle;
  auto generate_train_id = [station_count, &train_info_handle, this] {
    auto size = station_count - 1;
    train_info_handle = vls_->Allocate<
      TrainInfo>(size);
    return train_info_handle.RecordID();
  };
  if (!train_id_index_.GetOrEmplace(storage::Hash()(train_name),
                                    generate_train_id)) {
    return utils::FastIO::WriteFailure();
  }
  TrainInfo* train_info = train_info_handle.GetMut();
  // 2. Fill the train information
  utils::set_field(train_info->train_name, train_name,
                   sizeof(train_info->train_name));
  train_info->type = type;
  train_info->station_count = station_count;
  train_info->seat_count = seat_count;
  train_info->depart_time = depart_time;
  train_info->date_beg = utils::Parser::ParseDate(*sell_dates);
  train_info->date_end = utils::Parser::ParseDate(*++sell_dates);
  train_info->depart_station = GetStationId(*stations);
  // Set the vacancy id to INVALID_RECORD_ID because the train has not been released
  std::fill_n(train_info->vacancy_id, DATE_BATCH_COUNT,
              storage::INVALID_RECORD_ID);
  // 3. Fill the station information
  time_t total_time = 0;
  int total_price = 0;
  ++stations; // skip the departing station
  for (int i = 0; i < station_count - 1; ++i) {
    // the train will be added to the station's train list after it is released
    auto station_id = GetStationId(*stations);
    auto price = utils::stoi(*prices);
    auto travel_time = utils::stoi(*travel_times);
    auto stopover_time = i == station_count - 2
                           ? 0
                           : utils::stoi(*stopover_times);
    total_time += travel_time + stopover_time;
    total_price += price;
    train_info->station[i].station_id = station_id;
    train_info->station[i].arrive_time = total_time - stopover_time;
    train_info->station[i].leave_time = total_time;
    train_info->station[i].price = total_price;
    ++stations;
    ++prices;
    ++travel_times;
    ++stopover_times;
  }
  utils::FastIO::WriteSuccess();
}
void TrainManager::DeleteTrain(std::string_view train_name) {
  storage::record_id_t train_id;
  auto train_id_hash = storage::Hash()(train_name);
  if (!train_id_index_.GetValue(train_id_hash, &train_id)) {
    return utils::FastIO::WriteFailure();
  }
  if (vls_->Get<TrainInfo>(train_id).Get()->IsReleased()) {
    return utils::FastIO::WriteFailure();
  }
  train_id_index_.Remove(train_id_hash);
  // the VLS does not support deletion, so we just do nothing here
  utils::FastIO::WriteSuccess();
}
void TrainManager::ReleaseTrain(std::string_view train_name) {
  storage::record_id_t train_id = -1;
  if (!train_id_index_.GetValue(storage::Hash()(train_name), &train_id)) {
    return utils::FastIO::WriteFailure();
  }
  auto train_info = vls_->Get<TrainInfo>(train_id);
  if (train_info->IsReleased()) {
    return utils::FastIO::WriteFailure();
  }
  // 1. Add vacancy information
  size_t vacancy_size = DATE_BATCH_SIZE * (train_info->station_count - 1);
  for (int i = train_info->date_beg / DATE_BATCH_SIZE;
       i <= train_info->date_end / DATE_BATCH_SIZE; ++i) {
    auto vacancy = vls_->Allocate<Vacancy>(vacancy_size);
    train_info->vacancy_id[i] = vacancy.RecordID();
    std::fill_n(vacancy->vacancy, vacancy_size, train_info->seat_count);
  }
  // 2. Add the train to the station's train list
  station_train_index_.Insert({train_info->depart_station, train_id}, 'a');
  for (int i = 0; i < train_info->station_count - 1; ++i) {
    station_train_index_.Insert({train_info->station[i].station_id, train_id},
                                'a');
  }
  utils::FastIO::WriteSuccess();
}
void TrainManager::QueryTrain(std::string_view train_name, date_t date) {
  storage::record_id_t train_id;
  if (!train_id_index_.GetValue(storage::Hash()(train_name), &train_id)) {
    return utils::FastIO::WriteFailure();
  }
  const auto train_info_handle = vls_->Get<TrainInfo>(train_id);
  auto train_info = train_info_handle.Get();
  if (!train_info->IsOnSale(date)) {
    return utils::FastIO::WriteFailure();
  }
  /**
  查询成功：输出共 `(<stationNum> + 1)` 行。

  第一行为 `<trainID> <type>`。

  接下来 `<stationNum>` 行，第 `i` 行为 `<stations[i]> <ARRIVING_TIME> -> <LEAVING_TIME> <PRICE> <SEAT>`，其中 `<ARRIVING_TIME>` 和 `<LEAVING_TIME>` 为列车到达本站和离开本站的绝对时间，格式为 `mm-dd hr:mi`。`<PRICE>` 为从始发站乘坐至该站的累计票价，`<SEAT>` 为从该站到下一站的剩余票数。对于始发站的到达时间和终点站的出发时间，所有数字均用 `x` 代替；终点站的剩余票数用 `x` 代替。如果车辆还未 `release` 则认为所有票都没有被卖出去。
        */
  // 1. Output the train id and type
  utils::FastIO::Write(train_name, ' ', train_info->type, '\n');
  // 2. Output the station information
  bool released = train_info->IsReleased();
  storage::VarLengthStore::Handle<Vacancy> vacancy_handle;
  if (released) {
    vacancy_handle = vls_->Get<Vacancy>(train_info->GetVacancyId(date));
  }
  const Vacancy* vacancy = released ? vacancy_handle.Get() : nullptr;
  for (int8_t i = 0; i < train_info->station_count; ++i) {
    auto station_id = train_info->GetStationId(i);
    const auto station_name = vls_->Get<StationName>(station_id);
    abs_time_t arrive_time = train_info->GetArriveTime(date, i);
    abs_time_t leave_time = train_info->GetLeaveTime(date, i);
    int price = train_info->GetPrice(i);
    int seat = train_info->seat_count;
    if (released && i != train_info->station_count - 1) {
      seat = vacancy->GetVacancy(train_info->station_count, date, i);
    }
    utils::FastIO::Write(station_name->name, ' ',
                         utils::Parser::DateTimeString(arrive_time), " -> ",
                         utils::Parser::DateTimeString(leave_time), ' ', price,
                         ' ');
    if (i != train_info->station_count - 1) {
      utils::FastIO::Write(seat, '\n');
    }
  }
  utils::FastIO::Write("x\n");
}
void TrainManager::QueryTicket(std::string_view from_str,
                               std::string_view to_str,
                               date_t date, std::string_view sort_by) {
  storage::record_id_t from, to;
  if (!station_id_index_.GetValue(storage::Hash()(from_str), &from) ||
      !station_id_index_.GetValue(storage::Hash()(to_str), &to)) {
    return utils::FastIO::WriteFailure();
  }
  std::vector<std::pair<int, storage::record_id_t> > trains;
  auto from_it = station_train_index_.LowerBound(
      {from, storage::INVALID_RECORD_ID});
  auto to_it = station_train_index_.LowerBound(
      {to, storage::INVALID_RECORD_ID});
  bool sort_by_cost = sort_by == "cost"; // otherwise sort by time
  for (; from_it != station_train_index_.End() && from_it.Key().first == from;
         ++from_it) {
    while (to_it != station_train_index_.End() && to_it.Key().first == to
           && to_it.Key().second < from_it.Key().second)
      ++to_it;
    if (to_it == station_train_index_.End() || to_it.Key().first != to) {
      break;
    }
    if (from_it.Key().second != to_it.Key().second) continue;
    storage::record_id_t train_id = from_it.Key().second;
    auto train_handle = vls_->Get<TrainInfo>(train_id);
    auto train = train_handle.Get();
    auto from_station_no = train->GetStationNo(from);
    ASSERT(from_station_no != -1);
    if (!train->IsOnSale(train->GetDepartDate(date, from_station_no))) continue;
    auto to_station_no = train->GetStationNo(to);
    ASSERT(to_station_no != -1);
    if (sort_by_cost) {
      int cost = train->GetPrice(from_station_no, to_station_no);
      trains.emplace_back(cost, train_id);
    } else /* sort by time */ {
      int time = train->GetTravelTime(from_station_no, to_station_no);
      trains.emplace_back(time, train_id);
    }
  }
  storage::sort(trains.data(), trains.data() + trains.size());
  /*
  第一行输出一个整数，表示符合要求的车次数量。

  接下来每一行输出一个符合要求的车次，按要求排序。格式为 `<trainID> <FROM> <LEAVING_TIME> -> <TO> <ARRIVING_TIME> <PRICE> <SEAT>`，其中出发时间、到达时间格式同 `query_train`，`<FROM>` 和 `<TO>` 为出发站和到达站，`<PRICE>` 为累计价格，`<SEAT>` 为最多能购买的票数。
  */
  utils::FastIO::Write(trains.size());
  for (auto [_, train_id] : trains) {
    auto train_handle = vls_->Get<TrainInfo>(train_id);
    auto train = train_handle.Get();
    auto from_station_no = train->GetStationNo(from);
    auto to_station_no = train->GetStationNo(to);
    date_t depart_date = train->GetDepartDate(date, from_station_no);
    auto depart_time = train->GetLeaveTime(depart_date, from_station_no);
    auto arrive_time = train->GetArriveTime(depart_date, to_station_no);
    auto price = train->GetPrice(from_station_no, to_station_no);
    auto seat = train->seat_count;
    if (train->IsReleased()) {
      auto vacancy_handle = vls_->Get<Vacancy>(
          train->GetVacancyId(depart_date));
      seat = vacancy_handle.Get()->
          GetVacancy(train->station_count, date,
                     from_station_no, to_station_no);
    }
    utils::FastIO::Write(train->train_name, ' ',
                         from_str, ' ',
                         utils::Parser::DateTimeString(depart_time), " -> ",
                         to_str, ' ',
                         utils::Parser::DateTimeString(arrive_time), ' ',
                         price, ' ', seat, '\n');
  }
}
storage::record_id_t TrainManager::GetStationId(std::string_view station_name) {
  storage::record_id_t station_id;
  auto generate_station_id = [station_name, &station_id, this] {
    // include '\0'
    auto handle = vls_->Allocate<StationName>(station_name.size() + 1);
    station_id = handle.RecordID();
    utils::set_field(handle->name, station_name, station_name.size() + 1);
    return station_id;
  };
  station_id_index_.GetOrEmplace(storage::Hash()(station_name),
                                 generate_station_id, &station_id);
  return station_id;
}
} // namespace business