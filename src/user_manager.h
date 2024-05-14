//
// Created by zj on 5/13/2024.
//

#pragma once
#include <b_plus_tree.h>
#include <config.h>
#include <cstdint>
#include <variable_length_store.h>

namespace business {
struct UserProfile {
  using data_t = char;
  int order_count;
  // char username[30]; // no need to store username in user profile
  // char password[30];
  storage::hash_t password;
  char real_name[15];
  char email[30];
  int8_t privilege;
};

// User data stored in memory
struct UserData {
  storage::record_id_t user_id;
  int order_count;
  int8_t privilege;
};

class UserManager {
  public:
    UserManager(storage::BufferPoolManager<storage::VLS_PAGES_PER_FRAME> *bpm,
                storage::VarLengthStore *vls,
                bool reset) : vls_(vls), user_id_index_(bpm, bpm->AllocateInfo(), reset) {
    }

  void AddUser(std::string_view cur_user, std::string_view username, std::string_view password, std::string_view real_name, std::string_view email, int8_t privilege);

  void Login(std::string_view username, std::string_view password);

  private:
    storage::VarLengthStore *vls_;

  protected:
    std::unordered_map<storage::hash_t, UserData> logged_in_users_{};
    storage::BPlusTree<storage::hash_t, storage::record_id_t> user_id_index_;

  int8_t GetLoggedInUserPrivilege(storage::hash_t username);
  int8_t GetLoggedInUserPrivilege(std::string_view username);
};
} // namespace business
