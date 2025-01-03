#include <chrono>
#include <gtest/gtest.h>
#include <random>
#include <thread>
#include <vector>

#include "../include/dns_cache_testable.h"

TEST(DNSCacheTest, EmptyCache) {
  test::DNSCacheTestable cache(5);
  EXPECT_EQ(cache.resolve("example.com"), "");
}

TEST(DNSCacheTest, AddAndResolve) {
  test::DNSCacheTestable cache(5);
  cache.update("example.com", "192.168.1.1");
  EXPECT_EQ(cache.resolve("example.com"), "192.168.1.1");
}

TEST(DNSCacheTest, UpdateExisting) {
  test::DNSCacheTestable cache(5);
  cache.update("example.com", "192.168.1.1");
  cache.update("example.com", "192.168.1.2");
  EXPECT_EQ(cache.resolve("example.com"), "192.168.1.2");
}

TEST(DNSCacheTest, LRUEviction) {
  test::DNSCacheTestable cache(3);
  cache.update("site1.com", "1.1.1.1");
  cache.update("site2.com", "2.2.2.2");
  cache.update("site3.com", "3.3.3.3");
  cache.update("site4.com", "4.4.4.4");

  EXPECT_EQ(cache.resolve("site1.com"), ""); // Should be evicted
  EXPECT_EQ(cache.resolve("site2.com"), "2.2.2.2");
  EXPECT_EQ(cache.resolve("site3.com"), "3.3.3.3");
  EXPECT_EQ(cache.resolve("site4.com"), "4.4.4.4");
}

TEST(DNSCacheTest, LRUOrder) {
  test::DNSCacheTestable cache(3);
  cache.update("site1.com", "1.1.1.1");
  cache.update("site2.com", "2.2.2.2");
  cache.update("site3.com", "3.3.3.3");

  // Access site1 to make it most recently used
  cache.resolve("site1.com");

  cache.update("site4.com", "4.4.4.4");
  EXPECT_EQ(cache.resolve("site1.com"), "1.1.1.1");
  EXPECT_EQ(cache.resolve("site2.com"), ""); // Should be evicted
  EXPECT_EQ(cache.resolve("site3.com"), "3.3.3.3");
  EXPECT_EQ(cache.resolve("site4.com"), "4.4.4.4");
}

TEST(DNSCacheTest, MaxSizeRespected) {
  test::DNSCacheTestable cache(1000);
  for (int i = 0; i < 2000; ++i) {
    cache.update("site" + std::to_string(i) + ".com",
                 "1.1.1." + std::to_string(i));
  }
  int count = 0;
  for (int i = 0; i < 2000; ++i) {
    if (cache.resolve("site" + std::to_string(i) + ".com") != "") {
      ++count;
    }
  }
  EXPECT_EQ(count, 1000);
}

TEST(DNSCacheTest, ConcurrentAccess) {
  test::DNSCacheTestable cache(1000);
  std::vector<std::thread> threads;

  for (int i = 0; i < 10; ++i) {
    threads.emplace_back([&cache, i]() {
      for (int j = 0; j < 100; ++j) {
        std::string site = "site" + std::to_string(i * 100 + j) + ".com";
        std::string ip = std::to_string(i) + "." + std::to_string(j) + ".0.1";
        cache.update(site, ip);
        EXPECT_EQ(cache.resolve(site), ip);
      }
    });
  }

  for (auto &thread : threads) {
    thread.join();
  }

  // Verify some random entries
  EXPECT_EQ(cache.resolve("site50.com"), "0.50.0.1");
  EXPECT_EQ(cache.resolve("site750.com"), "7.50.0.1");
}

TEST(DNSCacheTest, ConcurrentReadsAndWrites) {
  test::DNSCacheTestable cache(1000);
  std::vector<std::thread> threads;

  auto update_func = [&cache](int start, int end) {
    for (int i = start; i < end; ++i) {
      cache.update("site" + std::to_string(i) + ".com",
                   "1.1.1." + std::to_string(i));
    }
  };

  auto resolve_func = [&cache](int start, int end) {
    for (int i = start; i < end; ++i) {
      cache.resolve("site" + std::to_string(i) + ".com");
    }
  };

  for (int i = 0; i < 4; ++i) {
    threads.emplace_back(resolve_func, i * 250, (i + 1) * 250);
    threads.emplace_back(update_func, i * 250, (i + 1) * 250);
  }

  for (auto &thread : threads) {
    thread.join();
  }

  int count = 0;
  for (int i = 0; i < 1000; ++i) {
    if (cache.resolve("site" + std::to_string(i) + ".com") != "") {
      ++count;
    }
  }
  EXPECT_EQ(count, 1000);
}

TEST(DNSCacheTest, ConcurrentUpdates) {
  test::DNSCacheTestable cache(100);
  std::vector<std::thread> threads;

  auto update_func = [&cache]() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 99);

    for (int i = 0; i < 1000; ++i) {
      int site_num = dis(gen);
      cache.update("site" + std::to_string(site_num) + ".com",
                   "1.1.1." + std::to_string(i));
    }
  };

  for (int i = 0; i < 10; ++i) {
    threads.emplace_back(update_func);
  }

  for (auto &thread : threads) {
    thread.join();
  }

  int count = 0;
  for (int i = 0; i < 100; ++i) {
    if (cache.resolve("site" + std::to_string(i) + ".com") != "") {
      ++count;
    }
  }
  EXPECT_EQ(count, 100);
}

TEST(DNSCacheTest, ConcurrentReadsWithTimeouts) {
  test::DNSCacheTestable cache(100);
  std::vector<std::thread> threads;

  for (int i = 0; i < 100; ++i) {
    cache.update("site" + std::to_string(i) + ".com",
                 "1.1.1." + std::to_string(i));
  }

  auto resolve_func = [&cache]() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 99);
    std::uniform_int_distribution<> sleep_dis(0, 100);

    for (int i = 0; i < 1000; ++i) {
      int site_num = dis(gen);
      cache.resolve("site" + std::to_string(site_num) + ".com");
      // Delay for emulation real conditions of working with DNS
      std::this_thread::sleep_for(std::chrono::milliseconds(sleep_dis(gen)));
    }
  };

  for (int i = 0; i < 20; ++i) {
    threads.emplace_back(resolve_func);
  }

  for (auto &thread : threads) {
    thread.join();
  }

  int count = 0;
  for (int i = 0; i < 100; ++i) {
    if (cache.resolve("site" + std::to_string(i) + ".com") != "") {
      ++count;
    }
  }
  EXPECT_EQ(count, 100);
}

TEST(DNSCacheTest, StressTest) {
  test::DNSCacheTestable cache(10000);
  std::vector<std::thread> threads;

  auto stress_func = [&cache]() {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 19999);
    std::uniform_int_distribution<> op_dis(0, 1);

    for (int i = 0; i < 100000; ++i) {
      int site_num = dis(gen);
      if (op_dis(gen) == 0) {
        cache.update("site" + std::to_string(site_num) + ".com",
                     "1.1.1." + std::to_string(i));
      } else {
        cache.resolve("site" + std::to_string(site_num) + ".com");
      }
    }
  };

  for (int i = 0; i < 50; ++i) {
    threads.emplace_back(stress_func);
  }

  for (auto &thread : threads) {
    thread.join();
  }

  int count = 0;
  for (int i = 0; i < 20000; ++i) {
    if (cache.resolve("site" + std::to_string(i) + ".com") != "") {
      ++count;
    }
  }
  EXPECT_LE(count, 10000);
  EXPECT_GT(count, 0);
}