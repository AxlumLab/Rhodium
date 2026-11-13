/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: error: error: git config user.name & please set dead value or install git && error: git config user.email & please set dead value or install git & please set dead value or install git
 * Copyright (c) 2026 by AxlumLab, All Rights Reserved.
 */

#pragma once

#include <limits>
#include <stdint.h>
#include <cstddef>

namespace rhodium::type
{

#ifndef MAX_PATH
    enum
    {
        MAX_PATH = 260
    };
#endif

    typedef int8_t i8;
    typedef uint8_t u8;
    typedef int16_t i16;
    typedef uint16_t u16;
    typedef int32_t i32;
    typedef uint32_t u32;
    typedef int64_t i64;
    typedef uint64_t u64;
    typedef float f32;
    typedef double f64;

    typedef size_t psize;
    typedef u8 byte;
    typedef char sbyte;
    typedef intptr_t intptr;

#if _WIN32
    typedef wchar_t wchar;
#else
    typedef char16_t wchar;
#endif
    typedef char c8;

    typedef u64 uintptr;
#define POINTER_SIZE 8

    static_assert(sizeof(i8) == 1, "Invalid i8 type size.");
    static_assert(sizeof(i16) == 2, "Invalid i16 type size.");
    static_assert(sizeof(i32) == 4, "Invalid i32 type size.");
    static_assert(sizeof(i64) == 8, "Invalid i64 type size.");
    static_assert(sizeof(u8) == 1, "Invalid u8 type size.");
    static_assert(sizeof(u16) == 2, "Invalid u16 type size.");
    static_assert(sizeof(u32) == 4, "Invalid u32 type size.");
    static_assert(sizeof(u64) == 8, "Invalid u64 type size.");
    static_assert(sizeof(wchar) == 2, "Invalid wchar type size.");
    static_assert(sizeof(char) == 1, "Invalid char type size.");
    static_assert(sizeof(bool) == 1, "Invalid bool type size.");
    static_assert(sizeof(f32) == 4, "Invalid f32 type size.");
    static_assert(sizeof(f64) == 8, "Invalid f64 type size.");

    constexpr i8 kMaxI8 = std::numeric_limits<i8>::max();
    constexpr i8 kMinI8 = std::numeric_limits<i8>::min();

    constexpr i16 kMaxI16 = std::numeric_limits<i16>::max();
    constexpr i16 kMinI16 = std::numeric_limits<i16>::min();

    constexpr i32 kMaxI32 = std::numeric_limits<i32>::max();
    constexpr i32 kMinI32 = std::numeric_limits<i32>::min();

    constexpr i64 kMaxI64 = std::numeric_limits<i64>::max();
    constexpr i64 kMinI64 = std::numeric_limits<i64>::min();

    constexpr u8 kMaxU8 = std::numeric_limits<u8>::max();
    constexpr u8 kMinU8 = std::numeric_limits<u8>::min();

    constexpr u16 kMaxU16 = std::numeric_limits<u16>::max();
    constexpr u16 kMinU16 = std::numeric_limits<u16>::min();

    constexpr u32 kMaxU32 = std::numeric_limits<u32>::max();
    constexpr u32 kMinU32 = std::numeric_limits<u32>::min();

    constexpr u64 kMaxU64 = std::numeric_limits<u64>::max();
    constexpr u64 kMinU64 = std::numeric_limits<u64>::min();

    constexpr psize kMaxPSize = std::numeric_limits<psize>::max();
    constexpr psize kMinPSize = std::numeric_limits<psize>::min();
    static_assert(sizeof(psize) == sizeof(void *), "psize is not vailed!");

    constexpr f32 kMaxF32 = std::numeric_limits<f32>::max();
    constexpr f32 kMinF32 = -std::numeric_limits<f32>::max();

    constexpr f64 kMaxF64 = std::numeric_limits<f64>::max();
    constexpr f64 kMinF64 = -std::numeric_limits<f64>::max();
    static_assert(sizeof(bool) == 1, "bool is not vailed!");

    constexpr i32 kInvalidIndex = -1;

    class TypeTraits
    {
    public:
        template <typename T, u32 count> static constexpr u32 lengthOf(const T (&)[count])
        {
            return count;
        }

        template <typename T> static constexpr T enumAddFlags(T value, T flags)
        {
            return (T)((__underlying_type(T))value | (__underlying_type(T))flags);
        }

        // Returns true if given enum value has one or more enum flags set
        template <typename T> static constexpr bool enumHasAnyFlags(T value, T flags)
        {
            return ((__underlying_type(T))value & (__underlying_type(T))flags) != 0;
        }

        // Returns true if given enum value has all of the enum flags set
        template <typename T> static constexpr bool enumHasAllFlags(T value, T flags)
        {
            return ((__underlying_type(T))value & (__underlying_type(T))flags) == (__underlying_type(T))flags;
        }
    };

    enum class BucketState : byte
    {
        kEmpty = 0,
        kDeleted,
        kOccupied,
    };

    inline constexpr psize operator""_B(unsigned long long int x)
    {
        return x;
    }

    inline constexpr psize operator""_KB(unsigned long long int x)
    {
        return x * 1024;
    }

    inline constexpr psize operator""_MB(unsigned long long int x)
    {
        return x * (1024 * 1024);
    }

    inline constexpr psize operator""_GB(unsigned long long int x)
    {
        return x * (1024 * 1024 * 1024);
    }

    inline constexpr u64 operator""_u64(unsigned long long arg) noexcept
    {
        return u64(arg);
    }

    inline constexpr f64 operator""_hour(long double x)
    {
        return f64(x) * 60.0;
    }

    inline constexpr f64 operator""_sec(long double x)
    {
        return f64(x);
    }

    inline constexpr f64 operator""_ms(long double x)
    {
        return f64(x) / 1000.0;
    }

    inline constexpr f64 operator""_ns(long double x)
    {
        return f64(x) / 1000000000.0;
    }
}

namespace rhodium
{
    using namespace rhodium::type;
}
