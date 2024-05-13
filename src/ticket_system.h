//
// Created by zj on 5/13/2024.
//

#pragma once

#include "ticket_manager.h"
#include "user_manager.h"
#include "train_manager.h"

namespace business {

class TicketSystem : public TicketManager, public UserManager, public TrainManager {

};

} // namespace business