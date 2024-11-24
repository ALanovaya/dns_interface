#include <list>
#include <mutex>
#include <shared_mutex>
#include <string>
#include <unordered_map>

class DNSCache {
private:
  struct CacheEntry {
    std::string ip;
    std::list<std::string>::iterator lru_iterator;
  };

  size_t max_size_;
  std::unordered_map<std::string, CacheEntry> cache_;
  std::list<std::string> lru_list_;
  mutable std::shared_mutex mutex_;

public:
  explicit DNSCache(size_t max_size);
  void update(const std::string &name, const std::string &ip);
  std::string resolve(const std::string &name);
};
