#pragma once

#include <map>
#include <mutex>

template <typename Key, typename Value>
class ConcurrentMap {
private:
    // Словарь с индивидуальным мутексом
    struct SubMap {
        std::mutex map_mutex;
        std::map<Key, Value> sub_map;
    };

public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys");

    struct Access {
        std::lock_guard<std::mutex> guard;
        Value& ref_to_value;
    };

    explicit ConcurrentMap(size_t bucket_count)
        : maps_{ bucket_count } {
    }

    Access operator[](const Key& key) {
        const uint64_t required_id = static_cast<uint64_t>(key) % static_cast<uint64_t>(maps_.size());
        SubMap& submap = maps_.at(required_id);

        return { std::lock_guard(submap.map_mutex), submap.sub_map[key] };
    }

    void Delete(const Key& key) {
        const uint64_t required_id = static_cast<uint64_t>(key) % static_cast<uint64_t>(maps_.size());
        SubMap& submap = maps_.at(required_id);

        std::lock_guard<std::mutex> guard(submap.map_mutex);
        submap.sub_map.erase(key);
    }

    std::map<Key, Value> BuildOrdinaryMap() {
        std::map<Key, Value> output;

        for (auto& map : maps_) {
            std::lock_guard<std::mutex> guard(map.map_mutex);
            output.insert(map.sub_map.begin(), map.sub_map.end());
        }

        return output;
    }

private:
    std::vector<SubMap> maps_; // Вектор словарей
};
