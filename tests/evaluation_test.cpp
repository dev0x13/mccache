#include <fstream>
#include <iostream>
#include <map>

#include <markov_chain_cache.h>

struct GetRequest {
    size_t timestamp;
    size_t itemId;
    size_t itemSize;
};

std::vector<GetRequest> parseTrace(std::ifstream& is) {
    size_t timestamp;
    size_t itemId;
    size_t itemSize;

    std::vector<GetRequest> requests;

    while (is >> timestamp >> itemId >> itemSize) {
        requests.push_back({ timestamp, itemId, itemSize });
    }

    return requests;
}

int main(int argc, char* argv[]) {

    if (argc < 5) {
        std::cout << "Usage: " << argv[0]
                  << " <path to trace file> <cache size> <stats accumulator type> "
                  << "<access threshold> <forecast length>"  << std::endl;
        return 1;
    }

    std::ifstream input(argv[1]);
    std::vector<GetRequest> trace = parseTrace(input);

    std::map<size_t, size_t> uniqueItems;

    for (const auto& r : trace) {
        uniqueItems[r.itemId] = r.itemSize;
    }

    MarkovChainCacheConfig cfg;

    cfg.cacheCapacity        = static_cast<float>(std::stoll(argv[2]));
    cfg.statsAccumulatorType = argv[3];
    cfg.accessesThreshold    = std::stoll(argv[4]);
    cfg.forecastLength       = std::stoll(argv[5]);

    MarkovChainCache<size_t> cache(cfg);

    for (const auto& item : uniqueItems) {
        cache.processSetRequest(item.first, item.second);
    }

    cache.flush();

    size_t numHits = 0;
    double numHitsBytes = 0;
    double totalSize = 0;

    for (const auto& r : trace) {
        if (cache.processGetRequest(r.itemId)) {
            numHits++;
            numHitsBytes += r.itemSize;
        }
        totalSize += r.itemSize;
    }

    std::cout << "Object hit ratio: " <<  (float) numHits / trace.size() << std::endl;
    std::cout << "Byte hit ratio: "   <<  numHitsBytes / totalSize       << std::endl;

    return 0;
}