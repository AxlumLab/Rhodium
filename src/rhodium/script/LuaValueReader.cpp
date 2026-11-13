/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#include "rhodium/script/LuaValueReader.hpp"

#include "rhodium/core/StringUtil.hpp"

#include <sstream>

namespace rhodium
{

    std::string LuaValueReader::string(sol::table table, const char *key, const std::string &fallback)
    {
        sol::object value = table[key];
        if (value.is<std::string>())
        {
            return value.as<std::string>();
        }
        return fallback;
    }

    bool LuaValueReader::boolean(sol::table table, const char *key, bool fallback)
    {
        sol::object value = table[key];
        if (value.is<bool>())
        {
            return value.as<bool>();
        }
        return fallback;
    }

    int LuaValueReader::integer(sol::table table, const char *key, int fallback)
    {
        sol::object value = table[key];
        if (value.is<int>())
        {
            return value.as<int>();
        }
        return fallback;
    }

    bool LuaValueReader::has(sol::table table, const char *key)
    {
        return !table[key].is<sol::nil_t>();
    }

    std::string LuaValueReader::text(sol::object value)
    {
        if (value.is<std::string>())
        {
            return value.as<std::string>();
        }
        if (value.is<bool>())
        {
            return value.as<bool>() ? "ON" : "OFF";
        }
        if (value.is<int>())
        {
            return std::to_string(value.as<int>());
        }
        if (value.is<double>())
        {
            std::ostringstream stream;
            stream << value.as<double>();
            return stream.str();
        }
        return {};
    }

    std::vector<std::string> LuaValueReader::strings(sol::object value)
    {
        std::vector<std::string> values;
        if (!value.is<sol::table>())
        {
            return values;
        }
        sol::table array = value.as<sol::table>();
        for (const auto &item : array)
        {
            if (item.second.is<std::string>())
            {
                values.push_back(item.second.as<std::string>());
            }
        }
        return values;
    }

    std::vector<std::string> LuaValueReader::array(sol::table table, const char *key)
    {
        return LuaValueReader::strings(table[key]);
    }

    std::map<std::string, std::string> LuaValueReader::map(sol::table table, const char *key)
    {
        std::map<std::string, std::string> values;
        sol::object value = table[key];
        if (!value.is<sol::table>())
        {
            return values;
        }
        sol::table map = value.as<sol::table>();
        for (const auto &item : map)
        {
            if (item.first.is<std::string>())
            {
                values[item.first.as<std::string>()] = LuaValueReader::text(item.second);
            }
        }
        return values;
    }

    std::map<std::string, std::vector<std::string>> LuaValueReader::arrayMap(sol::table table, const char *key)
    {
        std::map<std::string, std::vector<std::string>> values;
        sol::object value = table[key];
        if (!value.is<sol::table>())
        {
            return values;
        }
        sol::table map = value.as<sol::table>();
        for (const auto &item : map)
        {
            if (item.first.is<std::string>() && item.second.is<sol::table>())
            {
                values[StringUtils::toLower(item.first.as<std::string>())] = LuaValueReader::strings(item.second);
            }
        }
        return values;
    }

    void LuaValueReader::setString(sol::table table, const char *key, std::string &destination)
    {
        sol::object value = table[key];
        if (value.is<std::string>())
        {
            destination = value.as<std::string>();
        }
    }

    void LuaValueReader::setBoolString(sol::table table, const char *key, std::string &destination)
    {
        sol::object value = table[key];
        if (value.is<bool>())
        {
            destination = value.as<bool>() ? "true" : "false";
        }
        else if (value.is<std::string>())
        {
            destination = value.as<std::string>();
        }
    }

    void LuaValueReader::setArray(sol::table table, const char *key, std::vector<std::string> &destination)
    {
        if (LuaValueReader::has(table, key))
        {
            destination = LuaValueReader::array(table, key);
        }
    }

    void LuaValueReader::setArrayOrMap(sol::table table, const char *key, std::vector<std::string> &arrayDestination,
                                       std::map<std::string, std::vector<std::string>> &mapDestination)
    {
        if (!LuaValueReader::has(table, key))
        {
            return;
        }
        sol::object value = table[key];
        if (value.is<sol::table>())
        {
            sol::table values = value.as<sol::table>();
            bool stringArray = false;
            for (const auto &item : values)
            {
                if (item.first.is<int>() && item.second.is<std::string>())
                {
                    stringArray = true;
                    break;
                }
            }
            if (stringArray)
            {
                arrayDestination = LuaValueReader::strings(value);
                mapDestination.clear();
            }
            else
            {
                mapDestination = LuaValueReader::arrayMap(table, key);
                arrayDestination.clear();
            }
        }
    }

} // namespace rhodium
