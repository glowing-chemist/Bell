#ifndef HASH_UTILS_HPP
#define HASH_UTILS_HPP

#include <functional>
#include <vector>


inline void hash_combine(std::size_t& seed) { }

template <typename T, typename... Rest>
inline void hash_combine(std::size_t& seed, const T& v, Rest... rest)
{
    std::hash<T> hasher{};
    seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
    hash_combine(seed, rest...);
}

template<typename T>
inline void hash_combine(std::size_t& seed, const std::vector<T>& array)
{
    for(const auto& elem : array)
    {
        hash_combine(seed, elem);
    }
}

#endif