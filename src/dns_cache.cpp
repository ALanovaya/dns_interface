#include "../include/dns_cache.h"

DNSCache::DNSCache(size_t max_size) : max_size_(max_size) {}

void DNSCache::update(const std::string &name, const std::string &ip) {
  std::unique_lock<std::shared_mutex> lock(mutex_);

  auto it = cache_.find(name);
  if (it != cache_.end()) {
    // Update existing entry
    it->second.ip = ip;
    lru_list_.erase(it->second.lru_iterator);
    lru_list_.push_front(name);
    it->second.lru_iterator = lru_list_.begin();
  } else {
    // Add new entry
    if (cache_.size() >= max_size_) {
      // Remove least recently used entry
      auto lru_name = lru_list_.back();
      cache_.erase(lru_name);
      lru_list_.pop_back();
    }
    lru_list_.push_front(name);
    cache_[name] = {ip, lru_list_.begin()};
  }
}

std::string DNSCache::resolve(const std::string &name) {
  std::unique_lock<std::shared_mutex> lock(mutex_);

  auto it = cache_.find(name);
  if (it != cache_.end()) {
    // Move the accessed entry to the front of LRU list
    lru_list_.erase(it->second.lru_iterator);
    lru_list_.push_front(name);
    it->second.lru_iterator = lru_list_.begin();
    return it->second.ip;
  }
  return "";
}