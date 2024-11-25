cmake_minimum_required(VERSION 3.10)
project(DNSCacheTest)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_package(GTest REQUIRED)

add_executable(dns_cache_test 
    src/dns_cache_base.cpp
    tests/test_dns_cache_singleton.cpp
    tests/test_dns_cache.cpp
)

target_include_directories(dns_cache_test PRIVATE include)

target_compile_options(dns_cache_test PRIVATE -fsanitize=thread)
target_link_libraries(dns_cache_test -fsanitize=thread GTest::GTest GTest::Main pthread)
