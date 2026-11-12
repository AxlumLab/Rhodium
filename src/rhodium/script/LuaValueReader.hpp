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

    class LuaValueReader
    {
    public:
        static std::string string(sol::table table, const char *key, const std::string &fallback = {});
        static bool boolean(sol::table table, const char *key, bool fallback);
        static int integer(sol::table table, const char *key, int fallback);
        static bool has(sol::table table, const char *key);
        static std::string text(sol::object value);
        static std::vector<std::string> strings(sol::object value);
        static std::vector<std::string> array(sol::table table, const char *key);
        static std::map<std::string, std::string> map(sol::table table, const char *key);
        static std::map<std::string, std::vector<std::string>> arrayMap(sol::table table, const char *key);
        static void setString(sol::table table, const char *key, std::string &destination);
        static void setBoolString(sol::table table, const char *key, std::string &destination);
        static void setArray(sol::table table, const char *key, std::vector<std::string> &destination);
        static void setArrayOrMap(sol::table table, const char *key, std::vector<std::string> &arrayDestination,
                                  std::map<std::string, std::vector<std::string>> &mapDestination);
    };

} // namespace rhodium
