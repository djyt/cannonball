/***************************************************************************
    Data Types.

    The Boost library is only used to enforce data type size at compile
    time.

    If you're sure the sizes are correct, it can be removed for your port.

    Copyright Chris White.
    See license.txt for more details.
***************************************************************************/

#pragma once

#include <boost/static_assert.hpp>

/** C99 Standard Naming */
#if defined(_MSC_VER)
    typedef signed char int8_t;
    typedef signed short int16_t;
    typedef signed int int32_t;
    typedef signed long long int64_t;

    typedef unsigned char uint8_t;
    typedef unsigned short uint16_t;
    typedef unsigned int uint32_t;
    typedef unsigned long long uint64_t;
#else
    #include <stdint.h>
#endif

/* Report typedef errors */
BOOST_STATIC_ASSERT_MSG(sizeof(int8_t)   == 1, "int8_t is not of the correct size" );
BOOST_STATIC_ASSERT_MSG(sizeof(int16_t)  == 2, "int16_t is not of the correct size");
BOOST_STATIC_ASSERT_MSG(sizeof(int32_t)  == 4, "int32_t is not of the correct size");
BOOST_STATIC_ASSERT_MSG(sizeof(int64_t)  == 8, "int64_t is not of the correct size");

BOOST_STATIC_ASSERT_MSG(sizeof(uint8_t)  == 1, "int8_t is not of the correct size" );
BOOST_STATIC_ASSERT_MSG(sizeof(uint16_t) == 2, "int16_t is not of the correct size");
BOOST_STATIC_ASSERT_MSG(sizeof(uint32_t) == 4, "int32_t is not of the correct size");
BOOST_STATIC_ASSERT_MSG(sizeof(uint64_t) == 8, "int64_t is not of the correct size");
