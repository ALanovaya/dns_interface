#pragma once

#include <list>
#include <mutex>
#include <string>
#include <unordered_map>
#include <functional> 

class DNSCacheBase {
private:
  struct CacheEntry {
    std::string ip;
    std::list<std::reference_wrapper<const std::string>>::iterator lru_iterator;
  };

  size_t max_size_;
  std::unordered_map<std::string, CacheEntry> cache_;
  std::list<std::reference_wrapper<const std::string>> lru_list_;
  mutable std::mutex mutex_;

protected:
  explicit DNSCacheBase(size_t max_size);

public:
  void update(const std::string &name, const std::string &ip);
  std::string resolve(const std::string &name);
  size_t getMaxSize() const;
};
