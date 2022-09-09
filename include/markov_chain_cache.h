#pragma once

#include <algorithm>
#include <numeric>
#include <string>
#include <unordered_map>
#include <vector>

#include "math/evolving_markov_chain.h"

template <typename KeyType>
class CacheDelegate {
 public:
  virtual void AdmitItem(const KeyType& key) const = 0;

  virtual void EvictItem(const KeyType& key) const = 0;
};

struct MarkovChainCacheConfig {
  float cache_capacity = 512;
  std::string stats_accumulator_type = "transitions";
  size_t accesses_threshold = 5;

  // Cache parameter for regulating Markov chain forecast length
  size_t forecast_length = 1;
};

template <typename KeyType>
class MarkovChainCache {
 public:
  bool ProcessGetRequest(const KeyType& key) {
    assert(key_to_state_map_.count(key) != 0);

    if (items_in_cache_sizes_.count(key) != 0) {
      // Element is already in cache, nothing to do
      UpdateTransitionStats(key);
      return true;
    }

    // The logic in this method is quite similar to the one in
    // `ProcessSetRequest` method, so check the detailed comments on logic in
    // `ProcessSetRequest`.

    const float item_size = items_not_in_cache_sizes_.at(key);
    const float space_to_free =
        (current_cache_size_ + item_size) - cfg_.cache_capacity;

    if (space_to_free > 0) {
      const size_t markov_chain_current_state = key_to_state_map_[key];
      const size_t markov_chain_num_states = markov_chain_.GetNumStates();

      Vector<float> costs(markov_chain_num_states, FillType::kZeros);
      Vector<float> wrapped_item_sizes(item_sizes_.data(), item_sizes_.size());

      if (cfg_.forecast_length == 1) {
        markov_chain_.PredictNextState(markov_chain_current_state, &costs);
      } else {
        Vector<float> state(markov_chain_num_states, FillType::kZeros);
        state(markov_chain_current_state) = 1;

        for (size_t i = 0; i < cfg_.forecast_length; ++i) {
          state = markov_chain_.PredictNextState(state);
          costs.AddElements(state);
        }
      }

      costs.MulElements(wrapped_item_sizes);

      std::vector<size_t> eviction_candidate_states(costs.GetSize());
      std::iota(eviction_candidate_states.begin(),
                eviction_candidate_states.end(), 0);

      std::sort(eviction_candidate_states.begin(),
                eviction_candidate_states.end(),
                [&](size_t i, size_t j) { return costs(i) < costs(j); });

      Evict(space_to_free, eviction_candidate_states);

      items_not_in_cache_sizes_.erase(key);
    }

    if (delegate_) {
      delegate_->AdmitItem(key);
    }

    items_in_cache_sizes_[key] = item_size;
    current_cache_size_ += item_size;
    UpdateTransitionStats(key);

    return false;
  }

  void ProcessSetRequest(const KeyType& key, float item_size) {
    assert(item_size <= cfg_.cache_capacity);
    assert(item_size > 0);

    // We register the new state corresponding to th element which we are saving
    // now beforehand to determine if we could save it on disk right away
    // without a need to free space in cache.
    AddNewState(key, item_size);

    const float space_to_free =
        (current_cache_size_ + item_size) - cfg_.cache_capacity;

    if (space_to_free > 0) {
      const size_t markov_chain_num_states = markov_chain_.GetNumStates();
      const size_t markov_chain_current_state =
          !prev_requested_item_key_state_ ? 0 : *prev_requested_item_key_state_;

      Vector<float> costs(markov_chain_num_states, FillType::kZeros);
      Vector<float> wrapped_item_sizes(item_sizes_.data(), item_sizes_.size());

      if (cfg_.forecast_length == 1) {
        // In this case we are able to use the more efficient way to make a
        // prediction
        markov_chain_.PredictNextState(markov_chain_current_state, &costs);

        // (markov_chain_num_states - 1) state is the state corresponding to the
        // dataset being saved. Transition probability to it is apparently zero,
        // but most likely we don't want to instantly move it to disk. Instead,
        // we "fix" the probability with a probability given by stats
        // accumulator.
        costs(markov_chain_num_states - 1) =
            markov_chain_.GetTransitionProbabilityFromAccumulator(
                markov_chain_current_state, markov_chain_num_states - 1);
      } else {
        // Fill the vector representing current state
        Vector<float> state(markov_chain_num_states, FillType::kZeros);
        state(markov_chain_current_state) = 1;

        // Make predictions regarding forecast_length, and sum the
        // probabilities. It is not that formal, but we interpret this as a
        // cumulative cost of replacing by mistake.
        for (size_t i = 0; i < cfg_.forecast_length; ++i) {
          state = markov_chain_.PredictNextState(state);
          costs.AddElements(state);
        }
      }

      // Weight probabilities by the corresponding element sizes
      costs.MulElements(wrapped_item_sizes);

      // Sort costs in the ascending order
      std::vector<size_t> eviction_candidates(costs.GetSize());
      std::iota(eviction_candidates.begin(), eviction_candidates.end(), 0);

      const size_t markov_chain_state_for_saving_item = key_to_state_map_[key];
      std::sort(eviction_candidates.begin(), eviction_candidates.end(),
                [&](size_t i, size_t j) {
                  return costs(i) < costs(j) ||
                         (costs(i) == costs(j) &&
                          i == markov_chain_state_for_saving_item);
                });

      // Elements are being unloaded according to their indexes in the `ind`
      // vector until the required number of bytes is freed. There might be a
      // situation when the cost of replacing the element currently being saved
      // is that small, so it is a candidate to replace. In such case there is
      // no sense of replacing any elements from cache, so we just place the
      // freshly added element to disk right away
      float size_accumulator = 0;

      for (const auto& i : eviction_candidates) {
        const KeyType& tmp_key = state_to_key_map_[i];

        if (items_in_cache_sizes_.count(tmp_key) != 0) {
          size_accumulator += items_in_cache_sizes_[tmp_key];
        }

        if (tmp_key == key) {
          break;
        }
      }

      if (size_accumulator <= space_to_free) {
        items_not_in_cache_sizes_[key] = item_size;
        return;
      }

      Evict(space_to_free, eviction_candidates);
    }

    if (delegate_) {
      delegate_->AdmitItem(key);
    }

    items_in_cache_sizes_[key] = item_size;
    current_cache_size_ += item_size;
  }

  void Flush() {
    items_not_in_cache_sizes_.insert(items_in_cache_sizes_.begin(),
                                     items_in_cache_sizes_.end());
    items_in_cache_sizes_.clear();
    current_cache_size_ = 0;
  }

  explicit MarkovChainCache(const MarkovChainCacheConfig& cfg,
                            CacheDelegate<KeyType>* delegate = nullptr)
      : cfg_(cfg),
        markov_chain_(cfg.stats_accumulator_type, cfg.accesses_threshold),
        delegate_(delegate) {}

  ~MarkovChainCache() { delete prev_requested_item_key_state_; }

 private:
  // Markov chain stuff
  void UpdateTransitionStats(const KeyType& key) {
    assert(key_to_state_map_.count(key) != 0);

    if (!prev_requested_item_key_state_) {
      markov_chain_.RegisterTransition(
          !prev_requested_item_key_state_ ? 0 : *prev_requested_item_key_state_,
          key_to_state_map_[key]);
      prev_requested_item_key_state_ = new KeyType;
    } else {
      markov_chain_.RegisterTransition(*prev_requested_item_key_state_,
                                       key_to_state_map_[key]);
    }

    *prev_requested_item_key_state_ = key_to_state_map_[key];
  }

  void AddNewState(const KeyType& key, float size) {
    assert(key_to_state_map_.count(key) == 0);
    assert(size > 0);

    key_to_state_map_[key] = markov_chain_.AddState();
    state_to_key_map_.push_back(key);
    item_sizes_.push_back(size);
  }

  // Frees require amount of bytes by unloading some elements from memory to
  // disk
  void Evict(float space_to_free,
             const std::vector<size_t>& items_to_evict_states) {
    assert(space_to_free <= cfg_.cache_capacity);
    assert(space_to_free > 0);

    float spaceFreed = 0;

    for (const auto& state : items_to_evict_states) {
      const KeyType& item_key = state_to_key_map_[state];

      if (items_in_cache_sizes_.count(item_key) == 0) {
        // DS is already on disk, skip this entry
        continue;
      }

      const float tmp_size = item_sizes_[key_to_state_map_[item_key]];
      items_not_in_cache_sizes_[item_key] = tmp_size;
      spaceFreed += tmp_size;

      if (delegate_) {
        delegate_->EvictItem(item_key);
      }

      items_in_cache_sizes_.erase(item_key);

      if (spaceFreed >= space_to_free) {
        current_cache_size_ -= spaceFreed;
        return;
      }
    }

    current_cache_size_ -= spaceFreed;
  }

  MarkovChainCacheConfig cfg_;

  std::unordered_map<KeyType, float> items_in_cache_sizes_;
  std::unordered_map<KeyType, float> items_not_in_cache_sizes_;

  EvolvingMarkovChain markov_chain_;

  float current_cache_size_ = 0;

  // We use this vector for storing element sizes, because we multiply
  // transitions probabilities by element sizes in element wise fashion in order
  // to obtain the costs of replacing by mistake. This vector allows to do it
  // without copying data. itemsInCacheSizes map is only used for O(1) search.
  std::vector<float> item_sizes_;

  std::unordered_map<KeyType, size_t> key_to_state_map_;
  std::vector<KeyType> state_to_key_map_;

  CacheDelegate<KeyType>* delegate_ = nullptr;

  // This field store the actual state of cache in terms of Markov chain
  KeyType* prev_requested_item_key_state_ = nullptr;
};
