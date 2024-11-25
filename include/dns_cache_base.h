#pragma once

#include <list>
#include <shared_mutex>
#include <string>
#include <unordered_map>

class DNSCacheBase {
private:
  struct CacheEntry {
    std::string ip;
    std::list<std::string>::iterator lru_iterator;
  };

  size_t max_size_;
  std::unordered_map<std::string, CacheEntry> cache_;
  std::list<std::string> lru_list_;
  mutable std::shared_mutex mutex_;

protected:
  explicit DNSCacheBase(size_t max_size);

public:
  virtual ~DNSCacheBase() = default;

  virtual void update(const std::string &name, const std::string &ip);
  virtual std::string resolve(const std::string &name);
  virtual size_t getMaxSize() const;
};
