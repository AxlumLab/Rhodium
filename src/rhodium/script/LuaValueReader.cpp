/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved. 
 */

#include "rhodium/script/LuaValueReader.hpp"

#include "rhodium/core/StringUtil.hpp"

#include <sstream>

namespace rhodium
{

    std::string getStringOr(sol::table table, const char *key, const std::string &fallback)
    {
        sol::object value = table[key];
        if (value.is<std::string>())
        {
            return value.as<std::string>();
        }
        return fallback;
    }

    bool getBoolOr(sol::table table, const char *key, bool fallback)
    {
        sol::object value = table[key];
        if (value.is<bool>())
        {
            return value.as<bool>();
        }
        return fallback;
    }

    int getIntOr(sol::table table, const char *key, int fallback)
    {
        sol::object value = table[key];
        if (value.is<int>())
        {
            return value.as<int>();
        }
        return fallback;
    }

    bool hasField(sol::table table, const char *key)
    {
        return !table[key].is<sol::nil_t>();
    }

    std::string objectToString(sol::object value)
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

    std::vector<std::string> getStringArrayFromValue(sol::object value)
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

    std::vector<std::string> getStringArrayOr(sol::table table, const char *key)
    {
        return getStringArrayFromValue(table[key]);
    }

    std::map<std::string, std::string> getStringMapOr(sol::table table, const char *key)
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
                values[item.first.as<std::string>()] = objectToString(item.second);
            }
        }
        return values;
    }

    std::map<std::string, std::vector<std::string>> getStringArrayMapOr(sol::table table, const char *key)
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
                values[StringUtils::toLower(item.first.as<std::string>())] = getStringArrayFromValue(item.second);
            }
        }
        return values;
    }

    void setStringIfPresent(sol::table table, const char *key, std::string &destination)
    {
        sol::object value = table[key];
        if (value.is<std::string>())
        {
            destination = value.as<std::string>();
        }
    }

    void setBoolStringIfPresent(sol::table table, const char *key, std::string &destination)
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

    void setStringArrayIfPresent(sol::table table, const char *key, std::vector<std::string> &destination)
    {
        if (hasField(table, key))
        {
            destination = getStringArrayOr(table, key);
        }
    }

    void setStringArrayOrMapIfPresent(sol::table table,
                                      const char *key,
                                      std::vector<std::string> &arrayDestination,
                                      std::map<std::string, std::vector<std::string>> &mapDestination)
    {
        if (!hasField(table, key))
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
                arrayDestination = getStringArrayFromValue(value);
                mapDestination.clear();
            }
            else
            {
                mapDestination = getStringArrayMapOr(table, key);
                arrayDestination.clear();
            }
        }
    }

} // namespace rhodium
