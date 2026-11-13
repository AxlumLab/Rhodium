/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#include "rhodium/core/StringUtil.hpp"

#include <algorithm>
#include <cctype>

namespace rhodium
{

    std::string StringUtils::toLower(std::string value)
    {
        std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c)
                       { return static_cast<char>(std::tolower(c)); });
        return value;
    }

    bool StringUtils::equalsIgnoreCase(const std::string &lhs, const std::string &rhs)
    {
        return toLower(lhs) == toLower(rhs);
    }

} // namespace rhodium
