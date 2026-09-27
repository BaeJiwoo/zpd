#ifndef ZPD_SERVERLIMITS_HPP
#define ZPD_SERVERLIMITS_HPP

#include <cstddef>

namespace ServerLimits {
inline constexpr std::size_t MaxQueuedEvents = 1024;
inline constexpr std::size_t PlayersPerSession = 2;
static_assert(PlayersPerSession >= 2 && PlayersPerSession <= 16);
} // namespace ServerLimits

#endif // ZPD_SERVERLIMITS_HPP
