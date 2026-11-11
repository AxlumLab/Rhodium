/*
 * @Author: cubevlmu khfahqp@gmail.com
 * @LastEditors: cubevlmu khfahqp@gmail.com
 * Copyright (c) 2026 by FlybirdGames, All Rights Reserved. 
 */

#include "rhodium/utils/Sha256.hpp"
#include "rhodium/common.hpp"

#include <array>
#include <cctype>
#include <fstream>
#include <iomanip>
#include <iterator>
#include <sstream>
#include <vector>

namespace rhodium
{
    constexpr std::array<u32, 64> k = {
        0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
        0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u, 0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
        0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu, 0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
        0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u, 0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
        0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u, 0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
        0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u, 0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
        0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
        0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u, 0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u};

    u32 _rotr(u32 value, int bits)
    {
        return (value >> bits) | (value << (32 - bits));
    }

    void _transform(std::array<u32, 8> &state, const u8 *chunk)
    {
        std::array<u32, 64> w{};
        for (int i = 0; i < 16; ++i)
        {
            w[i] = (static_cast<u32>(chunk[i * 4]) << 24) |
                   (static_cast<u32>(chunk[i * 4 + 1]) << 16) |
                   (static_cast<u32>(chunk[i * 4 + 2]) << 8) |
                   static_cast<u32>(chunk[i * 4 + 3]);
        }
        for (int i = 16; i < 64; ++i)
        {
            const auto s0 = _rotr(w[i - 15], 7) ^ _rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
            const auto s1 = _rotr(w[i - 2], 17) ^ _rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }

        auto a = state[0], b = state[1], c = state[2], d = state[3];
        auto e = state[4], f = state[5], g = state[6], h = state[7];
        for (int i = 0; i < 64; ++i)
        {
            const auto s1 = _rotr(e, 6) ^ _rotr(e, 11) ^ _rotr(e, 25);
            const auto ch = (e & f) ^ ((~e) & g);
            const auto temp1 = h + s1 + ch + k[i] + w[i];
            const auto s0 = _rotr(a, 2) ^ _rotr(a, 13) ^ _rotr(a, 22);
            const auto maj = (a & b) ^ (a & c) ^ (b & c);
            const auto temp2 = s0 + maj;
            h = g;
            g = f;
            f = e;
            e = d + temp1;
            d = c;
            c = b;
            b = a;
            a = temp1 + temp2;
        }
        state[0] += a;
        state[1] += b;
        state[2] += c;
        state[3] += d;
        state[4] += e;
        state[5] += f;
        state[6] += g;
        state[7] += h;
    }

    std::string sha256File(const std::filesystem::path &path, std::string *error)
    {
        std::ifstream input(path, std::ios::binary);
        if (!input)
        {
            if (error)
                *error = "failed to open file: " + path.string();
            return {};
        }

        std::array<u32, 8> state = {
            0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
            0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u};
        std::vector<u8> data((std::istreambuf_iterator<char>(input)), {});
        const u64 bitLength = static_cast<u64>(data.size()) * 8u;
        data.push_back(0x80);
        while ((data.size() % 64) != 56)
        {
            data.push_back(0);
        }
        for (int i = 7; i >= 0; --i)
        {
            data.push_back(static_cast<u8>((bitLength >> (i * 8)) & 0xffu));
        }
        for (psize offset = 0; offset < data.size(); offset += 64)
        {
            _transform(state, data.data() + offset);
        }

        std::ostringstream out;
        out << std::hex << std::setfill('0');
        for (const auto word : state)
        {
            out << std::setw(8) << word;
        }
        return out.str();
    }

    bool sha256Equals(const std::string &left, const std::string &right)
    {
        if (left.size() != right.size())
        {
            return false;
        }
        for (psize i = 0; i < left.size(); ++i)
        {
            if (std::tolower(static_cast<unsigned char>(left[i])) != std::tolower(static_cast<unsigned char>(right[i])))
            {
                return false;
            }
        }
        return true;
    }
}
