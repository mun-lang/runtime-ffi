#include <catch2/catch.hpp>
#include <mun/runtime.h>
#include <sstream>

/// Returns the absolute path to the munlib with the specified name
std::string get_munlib_path(std::string_view name) {
  std::stringstream ss;
  ss << MUN_TEST_DIR << name;
  return ss.str();
}

TEST_CASE("can construct runtime from .munlib") {
  mun::Error err;
  auto runtime = mun::make_runtime(get_munlib_path("sources/fibonacci.munlib"), &err);
  if(err) {
    WARN(err.message());
  }
  REQUIRE(runtime.has_value());
}

TEST_CASE("can invoke function from .munlib") {
  mun::Error err;
  auto runtime = mun::make_runtime(get_munlib_path("sources/fibonacci.munlib"), &err);
  REQUIRE(runtime.has_value());
  //mun::invoke_fn<uint64_t, uint64_t>(*runtime, "fibonacci", 5);
}