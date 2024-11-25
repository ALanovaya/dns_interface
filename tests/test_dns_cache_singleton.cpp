#include <atomic>
#include <gtest/gtest.h>
#include <string>
#include <thread>

#include "../include/dns_cache.h"

class DNSTest : public ::testing::Test {
protected:
  void SetUp() override { cache = &DNSCache::getInstance(100); }

  DNSCache *cache;
};

void updateCache(int thread_id, const std::string &name,
                 const std::string &ip) {
  DNSCache &dns_cache = DNSCache::getInstance();
  dns_cache.update(name, ip);
}

void resolveCache(int thread_id, const std::string &name,
                  std::atomic<int> &resolved_count) {
  DNSCache &dns_cache = DNSCache::getInstance();
  std::string ip = dns_cache.resolve(name);
  if (!ip.empty()) {
    resolved_count++;
  }
}

TEST_F(DNSTest, ConcurrentUpdateResolve) {
  const int num_threads = 20;
  std::vector<std::thread> threads;
  std::atomic<int> resolved_count(0);

  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back(updateCache, i, "example.com",
                         "192.168.1." + std::to_string(i));
  }

  for (int i = 0; i < num_threads; ++i) {
    threads.emplace_back(resolveCache, i, "example.com",
                         std::ref(resolved_count));
  }

  for (auto &thread : threads) {
    thread.join();
  }

  EXPECT_EQ(resolved_count.load(), num_threads);
}

// Only for my calmness
TEST(DNSCacheSingletonTest, SingletonUsage) {
  DNSCache &cache1 = DNSCache::getInstance(500);
  DNSCache &cache2 = DNSCache::getInstance();

  EXPECT_EQ(cache1.getMaxSize(), 500);
  EXPECT_EQ(cache2.getMaxSize(), 500);
  EXPECT_EQ(&cache1, &cache2);
}

TEST(DNSCacheSingletonTest, SingletonWithDifferentMaxSize) {
  DNSCache &cache1 = DNSCache::getInstance(500);
  DNSCache &cache2 = DNSCache::getInstance(1000);

  EXPECT_EQ(cache1.getMaxSize(), 500);
  EXPECT_EQ(cache2.getMaxSize(), 500);
  EXPECT_EQ(&cache1, &cache2);
}
