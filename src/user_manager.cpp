//
// Created by zj on 5/13/2024.
//

#include "user_manager.h"

#include <fastio.h>
#include <hash.h>

namespace business {
void UserManager::AddUser(const std::string& cur_user,
                          const std::string& username,
                          const std::string& password,
                          const std::string& real_name,
                          const std::string& email, int8_t privilege) {
  if (GetLoggedInUserPrivilege(cur_user) <= privilege) {
    utils::FastIO::WriteFailure();
  }
  auto username_hash = storage::Hash()(username);
  auto handle = vls_->Allocate<UserProfile>();
  user_id_index_.Insert(username_hash, handle.RecordID());
  utils::set_field(handle->password, password, 30);
  utils::set_field(handle->real_name, real_name, 15);
  utils::set_field(handle->email, email, 30);
  handle->order_count = 0;
  handle->privilege = privilege;
  utils::FastIO::WriteSuccess();
}
int8_t UserManager::GetLoggedInUserPrivilege(const std::string& cur_user) {
  auto it = logged_in_users_.find(cur_user);
  if (it == logged_in_users_.end()) return -1;
  return it->second.privilege;
}
} // namespace business