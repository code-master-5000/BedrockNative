#include "EntityCache.h"

#include <iostream>

int main()
{
    EntityCache cache;

    // Scan 1: player and cow appear.
    auto first = cache.Update({
        {0xA, 0, {0, 0, 0}},
        {0xB, 1, {5, 0, 0}}
    });

    if (cache.Count() != 2 ||
        first.appeared.size() != 2 ||
        !first.disappeared.empty())
    {
        std::cerr << "FAIL: First scan\n";
        return 1;
    }

    // Scan 2: player moves, cow disappears, zombie appears.
    auto second = cache.Update({
        {0xA, 0, {10, 0, 0}},
        {0xC, 2, {8, 0, 0}}
    });

    if (cache.Count() != 2 ||
        second.appeared.size() != 1 ||
        second.appeared[0].id != 0xC ||
        second.disappeared.size() != 1 ||
        second.disappeared[0].id != 0xB)
    {
        std::cerr << "FAIL: Entity removal or addition\n";
        return 1;
    }

    // Scan 3: all entities disappear.
    auto third = cache.Update({});

    if (cache.Count() != 0 ||
        third.disappeared.size() != 2)
    {
        std::cerr << "FAIL: Empty snapshot\n";
        return 1;
    }

    // Simulate a new Minecraft session.
    cache.Update({{0xA, 0, {1, 2, 3}}});
    cache.Reset();

    if (cache.Count() != 0)
    {
        std::cerr << "FAIL: Reset\n";
        return 1;
    }

    // The same ID in a new session must count as new.
    auto restarted = cache.Update({
        {0xA, 0, {1, 2, 3}}
    });

    if (restarted.appeared.size() != 1)
    {
        std::cerr << "FAIL: New session\n";
        return 1;
    }

    std::cout << "PASS: EntityCache tests\n";
    return 0;
}
