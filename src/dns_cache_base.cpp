#include "../include/dns_cache_base.h"
#include <mutex>

DNSCacheBase::DNSCacheBase(size_t max_size) : max_size_(max_size) {}

void DNSCacheBase::update(const std::string &name, const std::string &ip) {
  std::unique_lock<std::mutex> lock(mutex_);

  auto it = cache_.find(name);
  if (it != cache_.end()) {
    // Update existing entry
    it->second.ip = ip;
    // Move the accessed entry to the front of LRU list
    lru_list_.splice(lru_list_.begin(), lru_list_, it->second.lru_iterator);
  } else {
    // Add new entry
    if (cache_.size() >= max_size_) {
      // Remove least recently used entry
      auto lru_name = lru_list_.back();
      cache_.erase(lru_name);
      lru_list_.pop_back();
    }
    auto [cache_it, inserted] = cache_.emplace(name, CacheEntry{ip, {}});

    lru_list_.push_front(std::cref(cache_it->first));

    cache_it->second.lru_iterator = lru_list_.begin();
  }
}

std::string DNSCacheBase::resolve(const std::string &name) {
  std::unique_lock<std::mutex> lock(mutex_);

  auto it = cache_.find(name);
  if (it != cache_.end()) {
    // Move the accessed entry to the front of LRU list
    lru_list_.splice(lru_list_.begin(), lru_list_, it->second.lru_iterator);
    return it->second.ip;
  }
  return "";
}

size_t DNSCacheBase::getMaxSize() const { return max_size_; }
