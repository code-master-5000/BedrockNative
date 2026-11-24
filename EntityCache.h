
#pragma once

#include "EntityTracker.h"

#include <cstdint>
#include <unordered_map>
#include <vector>

struct EntityChanges
{
    std::vector<TrackedEntity> appeared;
    std::vector<TrackedEntity> disappeared;
};

class EntityCache
{
public:
    EntityChanges Update(
        const std::vector<TrackedEntity>& snapshot)
    {
        EntityChanges changes;

        std::unordered_map<uint32_t, TrackedEntity> next;

        for (const auto& entity : snapshot)
        {
            if (entities_.find(entity.id) == entities_.end())
            {
                changes.appeared.push_back(entity);
            }

            next[entity.id] = entity;
        }

        for (const auto& entry : entities_)
        {
            if (next.find(entry.first) == next.end())
            {
                changes.disappeared.push_back(entry.second);
            }
        }

        entities_.swap(next);
        return changes;
    }

    void Reset()
{
    std::unordered_map<uint32_t, TrackedEntity> empty;
    entities_.swap(empty);
}

    size_t Count() const
    {
        return entities_.size();
    }

private:
    std::unordered_map<uint32_t, TrackedEntity> entities_;
};
