
#include "SignatureScanner.h"

std::vector<size_t> SignatureScanner::FindAll(
    const std::vector<uint8_t>& data,
    const Pattern& pattern)
{
    std::vector<size_t> matches;

    if (pattern.empty() ||
        pattern.size() > data.size())
    {
        return matches;
    }

    for (size_t i = 0;
         i <= data.size() - pattern.size();
         ++i)
    {
        bool found = true;

        for (size_t j = 0;
             j < pattern.size();
             ++j)
        {
            int expected = pattern[j];

            if (expected < -1 || expected > 255)
                return {};

            // -1 is a wildcard.
            if (expected != -1 &&
                data[i + j] !=
                    static_cast<uint8_t>(expected))
            {
                found = false;
                break;
            }
        }

        if (found)
            matches.push_back(i);
    }

    return matches;
}
