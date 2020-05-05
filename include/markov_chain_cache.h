#pragma once

#include <unordered_map>
#include <numeric>
#include <algorithm>

#include "math/evolving_markov_chain.h"

template <typename KeyType>
class CacheDelegate {
public:
    virtual void admitItem(const KeyType& key) const = 0;

    virtual void evictItem(const KeyType& key) const = 0;
};

struct MarkovChainCacheConfig {
    float cacheCapacity = 512;
    std::string statsAccumulatorType = "transitions";
    size_t accessesThreshold = 5;

    // Cache parameter for regulating Markov chain forecast length
    size_t forecastLength = 1;
};

template <typename KeyType>
class MarkovChainCache {
public:
    bool processGetRequest(const KeyType& key) {
        assert(keyToStateMap.count(key) != 0);

        if (itemsInCacheSizes.count(key) != 0) {
            // DS is already in RAM, nothing to do
            updateTransitionStats(key);
            return true;
        }

        // The logic in this method is quite similar to the one in `processSaving` method,
        // so check the detailed comments on logic in `processSaving`.

        const float itemSize    = itemsNotInCacheSizes.at(key);
        const float spaceToFree = (currentCacheSize + itemSize) - cfg.cacheCapacity;

        if (spaceToFree > 0) {
            const size_t markovChainCurrentState = !prevRequestedItemKeyState ? 0 : *prevRequestedItemKeyState;
            const size_t markovChainNumStates    = markovChain.getNumStates();

            Vector<float> costs(markovChainNumStates, FillType::ZEROS);
            Vector<float> wrappedDsSizes(itemsSizes.data(), itemsSizes.size());

            if (cfg.forecastLength == 1) {
                markovChain.predictNextState(markovChainCurrentState, &costs);
            } else {
                Vector<float> state(markovChainNumStates, FillType::ZEROS);
                state(markovChainCurrentState) = 1;

                for (size_t i = 0; i < cfg.forecastLength; ++i) {
                    state = markovChain.predictNextState(state);
                    costs.addElements(state);
                }
            }

            costs.mulElements(wrappedDsSizes);

            std::vector<size_t> evictionCandidatesStates(costs.getSize());
            std::iota(evictionCandidatesStates.begin(), evictionCandidatesStates.end(), 0);

            std::sort(evictionCandidatesStates.begin(), evictionCandidatesStates.end(), [&](size_t i, size_t j) {
                return costs(i) < costs(j);
            });

            evict(spaceToFree, evictionCandidatesStates);

            itemsNotInCacheSizes.erase(key);
        }

        if (delegate) {
            delegate->admitItem(key);
        }

        itemsInCacheSizes[key] = itemSize;
        currentCacheSize += itemSize;
        updateTransitionStats(key);

        return false;
    }

    void processSetRequest(const KeyType& key, float itemSize) {
        assert(itemSize <= cfg.cacheCapacity);
        assert(itemSize > 0);

        // We register the new state corresponding to dataset which we are saving now
        // beforehand to determine if we could save it on disk right away without a need
        // to free space in cache.
        addNewState(key, itemSize);

        const float spaceToFree = (currentCacheSize + itemSize) - cfg.cacheCapacity;

        if (spaceToFree > 0) {
            const size_t markovChainNumStates    = markovChain.getNumStates();

            // (Markov chain state corresponding to dsIndex) == dsIndex - 1
            const size_t markovChainCurrentState = !prevRequestedItemKeyState ? 0 : *prevRequestedItemKeyState;

            Vector<float> costs(markovChainNumStates, FillType::ZEROS);
            Vector<float> wrappedDsSizes(itemsSizes.data(), itemsSizes.size());

            if (cfg.forecastLength == 1) {
                // In this case we are able to use the more efficient way to make a prediction
                markovChain.predictNextState(markovChainCurrentState, &costs);

                // (markovChainNumStates - 1) state is the state corresponding to the dataset being saved.
                // Transition probability to it is apparently zero, but most likely we don't want to instantly
                // move it to disk. Instead we "fix" the probability with a probability given by stats accumulator.
                costs(markovChainNumStates - 1) = markovChain.getTransitionProbabilityFromAccumulator(markovChainCurrentState, markovChainNumStates - 1);
            } else {
                // Fill the vector representing current state
                Vector<float> state(markovChainNumStates, FillType::ZEROS);
                state(markovChainCurrentState) = 1;

                // Make predictions regarding to forecastLength, and sum the probabilities.
                // It is not that formal, but we interpret this as a cumulative cost of replacing
                // by mistake.
                for (size_t i = 0; i < cfg.forecastLength; ++i) {
                    state = markovChain.predictNextState(state);
                    costs.addElements(state);
                }
            }

            // Weight probabilities by the corresponding dataset sizes
            costs.mulElements(wrappedDsSizes);

            // Sort costs in the ascending order
            std::vector<size_t> evictionCandidates(costs.getSize());
            std::iota(evictionCandidates.begin(), evictionCandidates.end(), 0);

            const size_t markovChainStateForSavingItem = keyToStateMap[key];
            std::sort(evictionCandidates.begin(), evictionCandidates.end(), [&](size_t i, size_t j) {
                return costs(i) < costs(j) || (costs(i) == costs(j) && i == markovChainStateForSavingItem);
            });

            // Datasets are being unloaded according to their indexes in the `ind` vector
            // until the required number of bytes is freed. There might be a situation when
            // the cost of replacing the dataset currently being saved is that small so
            // it is a candidate to replace. In such case there is no sense of replacing
            // any datasets from cache, so we just place the freshly added dataset to disk right away/
            float sizesAccumulator = 0;

            for (const auto& i : evictionCandidates) {
                if (itemsInCacheSizes.count(i) != 0) {
                    sizesAccumulator += itemsInCacheSizes[i];
                }

                if (i == key) {
                    break;
                }
            }

            if (sizesAccumulator <= spaceToFree) {
                itemsNotInCacheSizes[key] = itemSize;
                return;
            }

            evict(spaceToFree, evictionCandidates);
        }

        if (delegate) {
            delegate->admitItem(key);
        }

        itemsInCacheSizes[key] = itemSize;
        currentCacheSize += itemSize;
    }

    void flush() {
        itemsNotInCacheSizes.insert(itemsInCacheSizes.begin(), itemsInCacheSizes.end());
        itemsInCacheSizes.clear();
        currentCacheSize = 0;
    }

    explicit MarkovChainCache(const MarkovChainCacheConfig& cfg, CacheDelegate<KeyType> *delegate = nullptr)
        : cfg(cfg), markovChain(cfg.statsAccumulatorType, cfg.accessesThreshold), delegate(delegate) {}

    ~MarkovChainCache() {
        delete prevRequestedItemKeyState;
    }

private:
    // Markov chain stuff
    void updateTransitionStats(const KeyType& key) {
        assert(keyToStateMap.count(key) != 0);

        if (!prevRequestedItemKeyState) {
            markovChain.registerTransition(!prevRequestedItemKeyState ? 0 : *prevRequestedItemKeyState, keyToStateMap[key]);
            prevRequestedItemKeyState = new KeyType;
        } else {
            markovChain.registerTransition(*prevRequestedItemKeyState, keyToStateMap[key]);
        }

        *prevRequestedItemKeyState = keyToStateMap[key];
    }

    void addNewState(const KeyType& key, float size) {
        assert(keyToStateMap.count(key) == 0);
        assert(size > 0);

        keyToStateMap[key] = markovChain.addState();
        stateToKeyMap.push_back(key);
        itemsSizes.push_back(size);
    }

    // Frees require amount of bytes by unloading some datasets from memory to disk
    void evict(float spaceToFree, const std::vector<size_t>& itemsToEvictStates) {
        assert(spaceToFree <= cfg.cacheCapacity);
        assert(spaceToFree > 0);

        float spaceFreed = 0;

        for (const auto& state : itemsToEvictStates) {
            const KeyType& itemKey = stateToKeyMap[state];

            if (itemsInCacheSizes.count(itemKey) == 0) {
                // DS is already on disk, skip this entry
                continue;
            }

            const float tmpSize = itemsSizes[keyToStateMap[itemKey]];
            itemsNotInCacheSizes[itemKey] = tmpSize;
            spaceFreed += tmpSize;

            if (delegate) {
                delegate->evictItem(itemKey);
            }

            itemsInCacheSizes.erase(itemKey);

            if (spaceFreed >= spaceToFree) {
                currentCacheSize -= spaceFreed;
                return;
            }
        }

        currentCacheSize -= spaceFreed;
    }

    MarkovChainCacheConfig cfg;

    std::unordered_map<KeyType, float> itemsInCacheSizes;
    std::unordered_map<KeyType, float> itemsNotInCacheSizes;

    EvolvingMarkovChain markovChain;

    float currentCacheSize = 0;

    // We use this vector for storing dataset sizes, because we multiply transitions
    // probabilities by dataset sizes in element wise fashion in order to obtain the
    // costs of replacing by mistake. This vector allows to do it without copying data.
    // datasetsInMemorySizes map is only used for O(1) search.
    std::vector<float> itemsSizes;

    std::unordered_map<KeyType, size_t> keyToStateMap;
    std::vector<KeyType> stateToKeyMap;

    CacheDelegate<KeyType> *delegate = nullptr;

    // This field store the actual state of cache in terms of Markov chain
    KeyType *prevRequestedItemKeyState = nullptr;
};