#include <atomic>
#include <gtest/gtest.h>
#include <string>
#include <thread>

#include "../include/dns_cache.h"

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

TEST(DNSCacheSingletonTest, SingleInstanceCreation) {
  std::vector<DNSCache *> instances(100);
  std::atomic<bool> failed{false};

  auto test_singleton = [&](int index) {
    DNSCache *instance = &DNSCache::getInstance();
    if (instances[index] != nullptr && instance != instances[index]) {
      failed = true;
    }
    instances[index] = instance;
  };

  std::vector<std::thread> threads;
  for (int i = 0; i < 100; ++i) {
    threads.emplace_back(test_singleton, i);
  }

  for (auto &thread : threads) {
    thread.join();
  }

  EXPECT_FALSE(failed)
      << "Singleton instances are not consistent across threads";
}

TEST(DNSCacheSingletonTest, ConcurrentAccess) {
  std::atomic<int> update_count{0};
  std::vector<std::thread> threads;

  auto update_func = [&]() {
    for (int i = 0; i < 1000; ++i) {
      DNSCache::getInstance().update("site" + std::to_string(i),
                                     "192.168.1." + std::to_string(i));
      update_count++;
    }
  };

  for (int i = 0; i < 10; ++i) {
    threads.emplace_back(update_func);
  }

  for (auto &thread : threads) {
    thread.join();
  }
  EXPECT_EQ(update_count, 10 * 1000);
}

TEST(DNSCacheSingletonTest, ThreadSafeInitialization) {
  std::atomic<int> init_count{0};
  std::vector<std::thread> threads;

  auto init_func = [&]() {
    DNSCache::getInstance();
    init_count++;
  };

  for (int i = 0; i < 100; ++i) {
    threads.emplace_back(init_func);
  }

  for (auto &thread : threads) {
    thread.join();
  }
  EXPECT_EQ(init_count, 100);
}

TEST(DNSCacheSingletonTest, ConcurrentResolveAndUpdate) {
  std::vector<std::thread> threads;
  std::atomic<int> resolve_count{0};
  std::atomic<int> update_count{0};
  std::atomic<bool> test_passed{true};

  auto resolve_func = [&]() {
    for (int i = 0; i < 5000; ++i) {
      try {
        DNSCache &instance = DNSCache::getInstance();
        std::string site = "site" + std::to_string(i % 100);
        std::string result = instance.resolve(site);
        if (!result.empty()) {
          resolve_count++;
        }
      } catch (const std::exception &e) {
        test_passed = false;
        std::cerr << "Resolve error: " << e.what() << std::endl;
      }
    }
  };

  auto update_func = [&]() {
    for (int i = 0; i < 5000; ++i) {
      try {
        DNSCache &instance = DNSCache::getInstance();
        std::string site = "site" + std::to_string(i % 100);
        std::string ip = "192.168.1." + std::to_string(i);

        instance.update(site, ip);
        update_count++;
      } catch (const std::exception &e) {
        test_passed = false;
        std::cerr << "Update error: " << e.what() << std::endl;
      }
    }
  };

  for (int i = 0; i < 10; ++i) {
    threads.emplace_back(resolve_func);
  }

  for (int i = 0; i < 10; ++i) {
    threads.emplace_back(update_func);
  }

  for (auto &thread : threads) {
    thread.join();
  }

  EXPECT_TRUE(test_passed) << "Concurrent resolve and update failed";
  EXPECT_GT(resolve_count, 0) << "No successful resolves";
  EXPECT_GT(update_count, 0) << "No successful updates";
}

TEST(DNSCacheSingletonTest, ConcurrentResolveUpdateMixed) {
  std::vector<std::thread> threads;
  std::atomic<int> successful_updates{0};
  std::atomic<int> successful_resolves{0};

  auto mixed_operation_func = [&]() {
    for (int i = 0; i < 5000; ++i) {
      std::string site = "site" + std::to_string(i % 50);
      std::string ip = "192.168.1." + std::to_string(i);

      if (rand() % 2 == 0) {
        DNSCache::getInstance().update(site, ip);
        successful_updates++;
      } else {
        std::string resolved = DNSCache::getInstance().resolve(site);
        if (!resolved.empty()) {
          successful_resolves++;
        }
      }
    }
  };

  for (int i = 0; i < 20; ++i) {
    threads.emplace_back(mixed_operation_func);
  }

  for (auto &thread : threads) {
    thread.join();
  }

  EXPECT_GT(successful_updates, 0) << "No successful updates";
  EXPECT_GT(successful_resolves, 0) << "No successful resolves";
}