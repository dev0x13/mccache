#include <markov_chain_cache.h>

#include <fstream>
#include <iostream>
#include <map>

#ifdef USE_MKL
#include <mkl/mkl.h>
#endif

struct GetRequest {
  size_t timestamp;
  size_t item_id;
  size_t item_size;
};

std::vector<GetRequest> ParseTrace(std::ifstream& is) {
  size_t timestamp;
  size_t item_id;
  size_t item_size;

  std::vector<GetRequest> requests;

  while (is >> timestamp >> item_id >> item_size) {
    requests.push_back({timestamp, item_id, item_size});
  }

  return requests;
}

class ProgressBar {
 private:
  unsigned int ticks = 0;

  const unsigned int totalTicks;
  const unsigned int barWidth;
  const char completeChar   = '=';
  const char incompleteChar = ' ';

 public:
  ProgressBar(unsigned int total, unsigned int width) : totalTicks(total), barWidth(width) {}

  unsigned int operator++() { return ++ticks; }

  void display() const {
    float progress = (float) ticks / totalTicks;
    unsigned int pos = barWidth * progress;

    std::cout << (int) (progress * 100.0) <<  "%[";

    for (unsigned int i = 0; i < barWidth; ++i) {
      if (i < pos) {
        std::cout << completeChar;
      } else if (i == pos) {
        std::cout << ">";
      } else {
        std::cout << incompleteChar;
      }
    }

    std::cout << "] " << ticks << '/' << totalTicks << '\r';
    std::cout.flush();
  }

  void done() const {
    display();
    std::cout << std::endl;
  }
};

int main(int argc, char* argv[]) {
  if (argc < 5) {
    std::cout << "Usage: " << argv[0]
              << " <path to trace file> <cache size> <stats accumulator type> "
              << "<access threshold> <forecast length>" << std::endl;
    return 1;
  }

#ifdef USE_MKL
  mkl_set_num_threads(mkl_get_max_threads());
#endif

  std::ifstream input(argv[1]);
  std::vector<GetRequest> trace = ParseTrace(input);

  std::map<size_t, size_t> unique_items;

  for (const auto& r : trace) {
    unique_items[r.item_id] = r.item_size;
  }

  MarkovChainCacheConfig cfg;

  cfg.cache_capacity = static_cast<float>(std::stoll(argv[2]));
  cfg.stats_accumulator_type = argv[3];
  cfg.accesses_threshold = std::stoll(argv[4]);
  cfg.forecast_length = std::stoll(argv[5]);

  MarkovChainCache<size_t> cache(cfg);

  ProgressBar pb(unique_items.size(), 100);

  for (const auto& item : unique_items) {
    cache.ProcessSetRequest(item.first, item.second);
    ++pb;
    pb.display();
  }

  pb.done();

  cache.Flush();

  size_t num_hits = 0;
  double num_hits_bytes = 0;
  double total_size = 0;

  for (const auto& r : trace) {
    if (cache.ProcessGetRequest(r.item_id)) {
      num_hits++;
      num_hits_bytes += r.item_size;
    }
    total_size += r.item_size;
  }

  std::cout << "Object hit ratio: "
            << static_cast<float>(num_hits) / trace.size() << std::endl;
  std::cout << "Byte hit ratio: " << num_hits_bytes / total_size << std::endl;

  return 0;
}
