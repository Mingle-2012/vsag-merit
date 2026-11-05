#pragma once

#include <algorithm>
#include <cstdint>
#include <limits>
#include <utility>
#include <vector>

namespace vsag::cleann_prune {
// CleANN L2 occlude_list: squared distances, alpha rounds 1, 1.2, ...,
// maxc=750 and saturate_graph=false, as in its native adapter defaults.
template <class Distance>
std::vector<std::pair<float, uint32_t>>
select(std::vector<std::pair<float, uint32_t>> pool,
       size_t degree,
       float alpha,
       Distance distance) {
    std::sort(pool.begin(), pool.end());
    if (pool.size() > 750) {
        pool.resize(750);
    }
    std::vector<float> occlusion(pool.size(), 0.0F);
    std::vector<std::pair<float, uint32_t>> result;
    result.reserve(std::min(degree, pool.size()));
    for (float current = 1.0F; current <= alpha && result.size() < degree; current *= 1.2F) {
        for (size_t i = 0; i < pool.size() && result.size() < degree; ++i) {
            if (occlusion[i] > current) {
                continue;
            }
            occlusion[i] = std::numeric_limits<float>::max();
            result.push_back(pool[i]);
            for (size_t j = i + 1; j < pool.size(); ++j) {
                if (occlusion[j] > alpha) {
                    continue;
                }
                const float pair = distance(pool[i].second, pool[j].second);
                occlusion[j] = pair == 0 ? std::numeric_limits<float>::max()
                                         : std::max(occlusion[j], pool[j].first / pair);
            }
        }
    }
    return result;
}
}  // namespace vsag::cleann_prune
