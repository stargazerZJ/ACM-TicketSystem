//
// Created by zj on 5/13/2024.
//

#include "train_manager.h"

#include <hash.h>

namespace business {
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
  for (int i = 0; i < DATE_BATCH_COUNT; ++i) {
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