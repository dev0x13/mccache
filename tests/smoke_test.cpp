#include <markov_chain_cache.h>

#include <iostream>

class CustomDelegate : public CacheDelegate<size_t> {
  void AdmitItem(const size_t& key) const override {
    std::cout << "Admit: " << key << std::endl;
  }

  void EvictItem(const size_t& key) const override {
    std::cout << "Evict: " << key << std::endl;
  }
};

int main() {
  // Without delegate
  {
    MarkovChainCacheConfig cfg;

    MarkovChainCache<size_t> cache(cfg);

    for (size_t i = 0; i < 100; ++i) {
      cache.ProcessSetRequest(i, i + 1);
    }

    for (size_t i = 0; i < 100; ++i) {
      cache.ProcessGetRequest(i);
    }
  }

  // With delegate
  {
    CustomDelegate delegate;

    MarkovChainCacheConfig cfg;

    cfg.cache_capacity = 100;

    MarkovChainCache<size_t> cache(cfg, &delegate);

    for (size_t i = 0; i < 100; ++i) {
      cache.ProcessSetRequest(i, i + 1);
    }

    for (size_t i = 0; i < 100; ++i) {
      cache.ProcessGetRequest(i);
    }
  }
}
