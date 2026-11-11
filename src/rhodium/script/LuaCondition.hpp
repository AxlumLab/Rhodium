/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved. 
 */

#pragma once

#include <string>

namespace rhodium
{

    bool conditionMatches(const std::string &condition, const std::string &activePlatform);
    bool isValidCondition(const std::string &condition);

} // namespace rhodium
