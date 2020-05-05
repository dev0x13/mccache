#include <fstream>
#include <iostream>
#include <map>

#include <markov_chain_cache.h>

#ifdef USE_MKL
#include <mkl.h>
#endif

struct GetRequest {
    char type; // `g` - get, `s` - set
    size_t timestamp;
    size_t itemId;
    size_t itemSize;
};

std::vector<GetRequest> parseTrace(std::ifstream& is) {
    char type;
    size_t timestamp;
    size_t itemId;
    size_t itemSize;

    std::vector<GetRequest> requests;

    while (is >> type >> timestamp >> itemId >> itemSize) {
        requests.push_back({ type, timestamp, itemId, itemSize });
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

#ifdef USE_MKL
    mkl_set_num_threads(mkl_get_max_threads());
#endif

    std::ifstream input(argv[1]);
    std::vector<GetRequest> trace = parseTrace(input);

    MarkovChainCacheConfig cfg;

    cfg.cacheCapacity        = static_cast<float>(std::stoll(argv[2]));
    cfg.statsAccumulatorType = argv[3];
    cfg.accessesThreshold    = std::stoll(argv[4]);
    cfg.forecastLength       = std::stoll(argv[5]);

    MarkovChainCache<size_t> cache(cfg);

    size_t numHits = 0;
    size_t numGetRequests = 0;
    double numHitsBytes = 0;
    double totalSize = 0;

    for (const auto& r : trace) {
        switch (r.type) {
            case 's':
                cache.processSetRequest(r.itemId, r.itemSize);
                break;
            case 'g':
                if (cache.processGetRequest(r.itemId)) {
                    numHits++;
                    numHitsBytes += r.itemSize;
                }

                totalSize += r.itemSize;
                numGetRequests++;
                break;
            default:
                throw std::invalid_argument("Invalid action type");
        }
    }

    std::cout << "Object hit ratio: " <<  (float) numHits / numGetRequests   << std::endl;
    std::cout << "Byte hit ratio: "   <<  numHitsBytes / totalSize       << std::endl;

    return 0;
}