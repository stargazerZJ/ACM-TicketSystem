//
// Created by zj on 5/13/2024.
//

#pragma once
#include <buffer_pool_manager.h>
#include <b_plus_tree.h>
#include <utility.h>

#include "variable_length_store.h"

namespace business {
struct TrainInfo {
  DELETE_CONSTRUCTOR_AND_DESTRUCTOR(TrainInfo);
  char train_name[20];
  int8_t station_count;
  int seat_count;
  time_t depart_time;
  date_t date_beg;
  date_t date_end;
  storage::record_id_t depart_station;

  struct Station {
    DELETE_CONSTRUCTOR_AND_DESTRUCTOR(Station);
    storage::record_id_t station_id;
    storage::record_id_t vacancy_id;
    time_t arrive_time; // the duration between departing from the first station and arriving at this station
    time_t leave_time; // the duration between departing from the first station and departing from this station
    int price; // the ticket price from the first station to this station
  };

  Station station[0]; // departing station is not included
  using data_t = Station;
};

// Vacacy information of one service of a train
struct Vacancy {
  DELETE_CONSTRUCTOR_AND_DESTRUCTOR(Vacancy);
  int vacancy[0]; // Its size is equal to the `station count - 1`of the train
  using data_t = int;
  using zero_base_size = std::true_type;
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
                  std::string_view stations_str,
                  std::string_view prices_str,
                  std::string_view travel_times_str,
                  std::string_view stopover_times_str,
                  std::string_view sell_dates_str
    );

  private:
    storage::VarLengthStore *vls_; // stores TrainInfo, Vacancy, and StationName
    storage::BPlusTree<storage::hash_t, storage::record_id_t> train_id_index_;
    storage::BPlusTree<storage::hash_t, storage::record_id_t> station_id_index_;
    storage::BPlusTree<
      storage::PackedPair<storage::record_id_t, storage::record_id_t>, char> station_train_index_;
    // station -> trains passing by. The value char is not used
};
} // namespace business
