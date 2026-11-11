/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved.
 */

#pragma once

#include <sol/sol.hpp>

#include <map>
#include <string>
#include <vector>

namespace rhodium
{

    std::string getStringOr(sol::table table, const char *key, const std::string &fallback = {});
    bool getBoolOr(sol::table table, const char *key, bool fallback);
    int getIntOr(sol::table table, const char *key, int fallback);
    bool hasField(sol::table table, const char *key);
    std::string objectToString(sol::object value);
    std::vector<std::string> getStringArrayFromValue(sol::object value);
    std::vector<std::string> getStringArrayOr(sol::table table, const char *key);
    std::map<std::string, std::string> getStringMapOr(sol::table table, const char *key);
    std::map<std::string, std::vector<std::string>> getStringArrayMapOr(sol::table table, const char *key);
    void setStringIfPresent(sol::table table, const char *key, std::string &destination);
    void setBoolStringIfPresent(sol::table table, const char *key, std::string &destination);
    void setStringArrayIfPresent(sol::table table, const char *key, std::vector<std::string> &destination);
    void setStringArrayOrMapIfPresent(sol::table table,
                                      const char *key,
                                      std::vector<std::string> &arrayDestination,
                                      std::map<std::string, std::vector<std::string>> &mapDestination);

} // namespace rhodium
