#pragma once

#include "dns_cache_base.h"

namespace test {
class DNSCacheTestable : public DNSCacheBase {
public:
  explicit DNSCacheTestable(size_t max_size) : DNSCacheBase(max_size) {}
};
} // namespace test