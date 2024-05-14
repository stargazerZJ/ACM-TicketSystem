//
// Created by zj on 5/13/2024.
//

#include "user_manager.h"

#include <fastio.h>
#include <hash.h>

namespace business {
void UserManager::AddUser(std::string_view cur_user,
                          std::string_view username,
                          std::string_view password,
                          std::string_view real_name,
                          std::string_view email, int8_t privilege) {
  if (user_id_index_.Empty()) {
    privilege = 10;
  } else {
    if (GetLoggedInUserPrivilege(cur_user) <= privilege) {
      return utils::FastIO::WriteFailure();
    }
  }
  auto username_hash = storage::Hash()(username);
  auto handle = vls_->Allocate<UserProfile>();
  if (user_id_index_.Insert(username_hash, handle.RecordID()) == false) {
    return utils::FastIO::WriteFailure();
  }
  handle->password = storage::Hash()(password);
  utils::set_field(handle->real_name, real_name, 15);
  utils::set_field(handle->email, email, 30);
  handle->order_count = 0;
  handle->privilege = privilege;
  utils::FastIO::WriteSuccess();
}
void UserManager::Login(std::string_view username, std::string_view password) {
  auto username_hash = storage::Hash()(username);
  storage::record_id_t user_id;
  if (!user_id_index_.GetValue(username_hash, &user_id)) {
    return utils::FastIO::WriteFailure();
  }
  const auto handle = vls_->Get<UserProfile>(user_id);
  if (handle->password != storage::Hash()(password)) {
    return utils::FastIO::WriteFailure();
  }
  logged_in_users_[username_hash] = {user_id, handle->order_count,
                                     handle->privilege};
  utils::FastIO::WriteSuccess();
}
void UserManager::Logout(std::string_view username) {
  auto username_hash = storage::Hash()(username);
  if (logged_in_users_.erase(username_hash) == 0) {
    return utils::FastIO::WriteFailure();
  }
  utils::FastIO::WriteSuccess();
}
int8_t UserManager::GetLoggedInUserPrivilege(storage::hash_t username) {
  auto it = logged_in_users_.find(username);
  if (it == logged_in_users_.end()) return -1;
  return it->second.privilege;
}
int8_t UserManager::GetLoggedInUserPrivilege(std::string_view username) {
  return GetLoggedInUserPrivilege(storage::Hash()(username));
}
} // namespace business