
#pragma once

#include <cstdint>
#include <cstddef>
#include <vector>

class SignatureScanner
{
public:
    // -1 means wildcard (??).
    using Pattern = std::vector<int>;

    static std::vector<size_t> FindAll(
        const std::vector<uint8_t>& data,
        const Pattern& pattern
    );
};
