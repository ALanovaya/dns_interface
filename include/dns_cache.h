#pragma once

#include "dns_cache_base.h"

class DNSCache : public DNSCacheBase {
public:
  static DNSCache& getInstance(size_t max_size = 1000) {
    static DNSCache instance(max_size);
    return instance;
  }

  // Prevent copying and assignment
  DNSCache(const DNSCache &) = delete;
  DNSCache &operator=(const DNSCache &) = delete;
  DNSCache(DNSCache &&) = delete;
  DNSCache &operator=(DNSCache &&) = delete;

private:
  explicit DNSCache(size_t max_size) : DNSCacheBase(max_size) {}
};