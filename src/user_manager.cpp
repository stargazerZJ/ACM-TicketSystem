//
// Created by zj on 5/13/2024.
//

#include "user_manager.h"

#include <fastio.h>
#include <hash.h>

namespace business {
UserManager::~UserManager() {
  for (const auto &[_, user] : logged_in_users_) {
    if (user.order_count != user.original_order_count) {
      auto handle = vls_->Get<UserProfile>(user.user_id);
      handle->order_count = user.order_count;
    }
  }
}
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
  if (logged_in_users_.contains(username_hash)
      || !user_id_index_.GetValue(username_hash, &user_id)) {
    return utils::FastIO::WriteFailure();
  }
  const auto handle = vls_->Get<UserProfile>(user_id);
  if (handle->password != storage::Hash()(password)) {
    return utils::FastIO::WriteFailure();
  }
  logged_in_users_[username_hash] = {user_id, handle->order_count, handle->order_count,
                                     handle->privilege};
  utils::FastIO::WriteSuccess();
}
void UserManager::Logout(std::string_view username) {
  auto username_hash = storage::Hash()(username);
  auto it = logged_in_users_.find(username_hash);
  if (it == logged_in_users_.end()) {
    return utils::FastIO::WriteFailure();
  }
  // Write the number of orders of the user being logged out.
  if (it->second.order_count != it->second.original_order_count) {
    auto handle = vls_->Get<UserProfile>(it->second.user_id);
    handle->order_count = it->second.order_count;
  }
  logged_in_users_.erase(it);
  utils::FastIO::WriteSuccess();
}
void UserManager::QueryProfile(std::string_view cur_user,
                               std::string_view username) {
  auto cur_user_hash = storage::Hash()(cur_user);
  auto cur_user_privilege = GetLoggedInUserPrivilege(cur_user_hash);
  if (cur_user_privilege == -1) {
    return utils::FastIO::WriteFailure();
  }
  auto username_hash = storage::Hash()(username);
  storage::record_id_t user_id;
  if (!user_id_index_.GetValue(username_hash, &user_id)) {
    return utils::FastIO::WriteFailure();
  }
  const auto handle = vls_->Get<UserProfile>(user_id);
  if (cur_user_hash != username_hash && cur_user_privilege <= handle->
      privilege) {
    return utils::FastIO::WriteFailure();
  }
  // Query successful: output one line,
  // the `<username>`, `<name>`, `<mailAddr>` and `<privilege>`
  // of the user being queried, separated by a space.
  utils::FastIO::Write(username, ' ',
                       utils::get_field(handle->real_name, 15), ' ',
                       utils::get_field(handle->email, 30), ' ',
                       handle->privilege, '\n');
}
void UserManager::ModifyProfile(std::string_view cur_user,
                                std::string_view username,
                                std::string_view password,
                                std::string_view real_name,
                                std::string_view email, int8_t privilege) {
  auto cur_user_hash = storage::Hash()(cur_user);
  auto cur_user_privilege = GetLoggedInUserPrivilege(cur_user_hash);
  if (cur_user_privilege <= privilege) {
    return utils::FastIO::WriteFailure();
  }
  auto username_hash = storage::Hash()(username);
  storage::record_id_t user_id;
  if (!user_id_index_.GetValue(username_hash, &user_id)) {
    return utils::FastIO::WriteFailure();
  }
  auto handle = vls_->Get<UserProfile>(user_id);
  if (cur_user_hash != username_hash && cur_user_privilege <= handle.Get()->
      privilege) {
    return utils::FastIO::WriteFailure();
  }
  if (!password.empty()) {
    handle->password = storage::Hash()(password);
  }
  if (!real_name.empty()) {
    utils::set_field(handle->real_name, real_name, 15);
  }
  if (!email.empty()) {
    utils::set_field(handle->email, email, 30);
  }
  if (privilege != -1) {
    handle->privilege = privilege;
    auto it = GetLoggedInUser(username_hash);
    if (it != logged_in_users_.end()) {
      it->second.privilege = privilege;
    }
  }
  utils::FastIO::Write(username, ' ',
                       utils::get_field(handle->real_name, 15), ' ',
                       utils::get_field(handle->email, 30), ' ',
                       handle->privilege, '\n');
}
int8_t UserManager::GetLoggedInUserPrivilege(storage::hash_t username) {
  auto it = logged_in_users_.find(username);
  if (it == logged_in_users_.end()) return -1;
  return it->second.privilege;
}
int8_t UserManager::GetLoggedInUserPrivilege(std::string_view username) {
  return GetLoggedInUserPrivilege(storage::Hash()(username));
}
auto UserManager::GetLoggedInUser(
    storage::hash_t username) -> decltype(logged_in_users_.find({})) {
  return logged_in_users_.find(username);
}
auto UserManager::GetLoggedInUser(
    std::string_view username) -> decltype(logged_in_users_.find(
    {})) {
  return logged_in_users_.find(storage::Hash()(username));
}
} // namespace business