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

  storage::record_id_t GetVacancyId(date_t date) const { return vacancy_id[date / DATE_BATCH_SIZE]; }

  storage::record_id_t GetStationId(int station_no) const;

  int GetStationNo(storage::record_id_t station_id) const;

  abs_time_t GetArriveTime(date_t date, int station_no) const;

  abs_time_t GetLeaveTime(date_t date, int station_no) const;

  time_t GetTravelTime(int from, int to) const;

  int GetPrice(int station_no) const;

  int GetPrice(int from, int to) const;

  /// @brief Get the date when the train departs from the FIRST station
  /// @param leaving_date the date when the train departs from the `leaving_station`
  /// @param leaving_station the station where the train departs
  /// @retuen the date when the train departs from the FIRST station
  date_t GetDepartDate(date_t leaving_date, int leaving_station) const;
};

// Vacacy information of one service of a train
struct Vacancy {
  DELETE_CONSTRUCTOR_AND_DESTRUCTOR(Vacancy);
  int vacancy[0]; // Its size is equal to the `station count - 1`of the train
  using data_t = int;
  using zero_base_size = std::true_type;

  int GetVacancy(int8_t station_count, date_t date, int station_no) const;

  const int *GetVacancy(int8_t station_count, date_t date) const;

  int *GetVacancy(int8_t station_count, date_t date);

  int GetVacancy(int8_t station_count, date_t date, int from, int to) const;

  void ReduceVacancy(int8_t station_count, date_t date, int from, int to, int num);
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

    void QueryTicket(std::string_view from_str,
                     std::string_view to_str,
                     date_t date,
                     std::string_view sort_by // "time" (default) or "cost"
    );

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
