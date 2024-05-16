//
// Created by zj on 5/13/2024.
//

#pragma once
#include "buffer_pool_manager.h"
#include "b_plus_tree.h"
#include "parser.h"
#include "utility.h"
#include "variable_length_store.h"

namespace business {
struct TrainInfo {
  DELETE_CONSTRUCTOR_AND_DESTRUCTOR(TrainInfo);
  char train_name[20];
  char type; // 'D' or 'G'
  int8_t station_count;
  int seat_count;
  time_t depart_time;
  date_t date_beg;
  date_t date_end;
  storage::record_id_t depart_station;
  storage::record_id_t vacancy_id[DATE_BATCH_COUNT];

  struct Station {
    DELETE_CONSTRUCTOR_AND_DESTRUCTOR(Station);
    storage::record_id_t station_id;
    time_t arrive_time; // the duration between departing from the first station and arriving at this station
    time_t leave_time; // the duration between departing from the first station and departing from this station
    int price; // the ticket price from the first station to this station
  };

  Station station[0]; // departing station is not included, so size is `station_count - 1`
  using data_t = Station;

  bool IsReleased() const { return vacancy_id[0] != storage::INVALID_RECORD_ID; }

  bool IsOnSale(date_t date) const { return date >= date_beg && date <= date_end; }

  storage::record_id_t GetVacancyId(date_t date) const {
    return vacancy_id[date / DATE_BATCH_SIZE];
  }

  storage::record_id_t GetStationId(int station_id) const {
    if (station_id == 0) return depart_station;
    ASSERT(station_id < station_count);
    return station[station_id - 1].station_id;
  }

  abs_time_t GetArriveTime(date_t date, int station_id) const {
    if (station_id == 0) return -1;
    ASSERT(station_id < station_count);
    return date * 1440
        + depart_time + station[station_id - 1].arrive_time;
  }

  abs_time_t GetLeaveTime(date_t date, int station_id) const {
    if (station_id == 0) return date * 1440 + depart_time;
    ASSERT(station_id < station_count);
    if (station_id == station_count - 1) return -1;
    return date * 1440
        + depart_time + station[station_id].leave_time;
  }

  int GetPrice(int station_id) const {
    if (station_id == 0) return 0;
    ASSERT(station_id < station_count);
    return station[station_id - 1].price;
  }
};

// Vacacy information of one service of a train
struct Vacancy {
  DELETE_CONSTRUCTOR_AND_DESTRUCTOR(Vacancy);
  int vacancy[0]; // Its size is equal to the `station count - 1`of the train
  using data_t = int;
  using zero_base_size = std::true_type;

  int GetVacancy(int8_t station_count, date_t date, int station_id) const {
    ASSERT(station_id < station_count - 1);
    return vacancy[(date % DATE_BATCH_SIZE) * (station_count - 1) + station_id];
  }
};

struct StationName {
  DELETE_CONSTRUCTOR_AND_DESTRUCTOR(StationName);
  char name[0];
  using data_t = char;
  using zero_base_size = std::true_type;
};

class TrainManager {
  public:
    TrainManager(storage::BufferPoolManager<storage::BPT_PAGES_PER_FRAME> *bpm,
                 storage::VarLengthStore *vls,
                 bool reset) : vls_(vls),
                               train_id_index_(bpm, bpm->AllocateInfo(), reset),
                               station_id_index_(bpm, bpm->AllocateInfo(), reset),
                               station_train_index_(bpm, bpm->AllocateInfo(), reset) {
    }

    void AddTrain(std::string_view train_name,
                  int8_t station_count,
                  int seat_count,
                  time_t depart_time,
                  char type,
                  utils::Parser::DelimitedStrIterator stations,
                  utils::Parser::DelimitedStrIterator prices,
                  utils::Parser::DelimitedStrIterator travel_times,
                  utils::Parser::DelimitedStrIterator stopover_times,
                  utils::Parser::DelimitedStrIterator sell_dates
    );

    void DeleteTrain(std::string_view train_name);

    void ReleaseTrain(std::string_view train_name);

    void QueryTrain(std::string_view train_name, date_t date);

  private:
    storage::VarLengthStore *vls_; // stores TrainInfo, Vacancy, and StationName
    storage::BPlusTree<storage::hash_t, storage::record_id_t> train_id_index_;
    storage::BPlusTree<storage::hash_t, storage::record_id_t> station_id_index_;
    storage::BPlusTree<
      storage::PackedPair<storage::record_id_t, storage::record_id_t>, char> station_train_index_;
    // station -> trains passing by. The value char is not used

    storage::record_id_t GetStationId(std::string_view station_name); // Will create a new station if not found
};
} // namespace business
