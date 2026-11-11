/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved. 
 */

#pragma once

#include <string>

namespace rhodium
{

    class DiagnosticSink;

    void reportUnsupportedProjectDsl(DiagnosticSink &diagnostics, const std::string &name);

} // namespace rhodium
