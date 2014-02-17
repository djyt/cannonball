/***************************************************************************
    General C++ Helper Functions

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once

#include <string>
#include "stdint.hpp"

class Utils
{
public:
    static std::string to_string(int i);
    static std::string to_string(char c);
    static std::string to_hex_string(int i);
    static uint32_t from_hex_string(std::string s);

private:
};