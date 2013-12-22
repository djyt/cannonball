/***************************************************************************
    General C++ Helper Functions

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#include <sstream>
#include "utils.hpp"

// Convert value to string
std::string Utils::to_string(int i)
{
    std::stringstream ss;
    ss << i;
    return ss.str();
}

// Convert value to string
std::string Utils::to_string(char c)
{
    std::stringstream ss;
    ss << c;
    return ss.str();
}

// Convert value to string
//template<class T>
std::string Utils::to_hex_string(int i)
{
    std::stringstream ss;
    ss << std::hex << i;
    return ss.str();
}

// Convert hex string to unsigned int
uint32_t Utils::from_hex_string(std::string s)
{
    unsigned int x;   
    std::stringstream ss;
    ss << std::hex << s;
    ss >> x;
    // output it as a signed type
    return static_cast<unsigned int>(x);
}