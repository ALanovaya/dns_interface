#include <gtest/gtest.h>

#include "../include/dns_cache.h"

// Only for my calmness
TEST(DNSCacheSingletonTest, SingletonUsage) {
  DNSCache& cache1 = DNSCache::getInstance(500);
  DNSCache& cache2 = DNSCache::getInstance();

  EXPECT_EQ(cache1.getMaxSize(), 500);
  EXPECT_EQ(cache2.getMaxSize(), 500);
  EXPECT_EQ(&cache1, &cache2);
}

TEST(DNSCacheSingletonTest, SingletonWithDifferentMaxSize) {
  DNSCache& cache1 = DNSCache::getInstance(500);
  DNSCache& cache2 = DNSCache::getInstance(1000);

  EXPECT_EQ(cache1.getMaxSize(), 500);
  EXPECT_EQ(cache2.getMaxSize(), 500);
  EXPECT_EQ(&cache1, &cache2);
}