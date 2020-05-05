#include <iostream>

#include <markov_chain_cache.h>

class CustomDelegate : public CacheDelegate<size_t> {
    void admitItem(const size_t& key) const override {
        std::cout << "Admit: " << key << std::endl;
    }

    void evictItem(const size_t& key) const override {
        std::cout << "Evict: " << key << std::endl;
    }
};

int main() {
    // Without delegate
    {
        MarkovChainCacheConfig cfg;

        MarkovChainCache<size_t> cache(cfg);

        for (size_t i = 0; i < 100; ++i) {
            cache.processSetRequest(i, i + 1);
        }

        for (size_t i = 0; i < 100; ++i) {
            cache.processGetRequest(i);
        }
    }

    // With delegate
    {
        CustomDelegate delegate;

        MarkovChainCacheConfig cfg;

        cfg.cacheCapacity = 100;

        MarkovChainCache<size_t> cache(cfg, &delegate);

        for (size_t i = 0; i < 100; ++i) {
            cache.processSetRequest(i, i + 1);
        }

        for (size_t i = 0; i < 100; ++i) {
            cache.processGetRequest(i);
        }
    }
}