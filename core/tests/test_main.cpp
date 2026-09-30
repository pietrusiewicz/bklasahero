// Punkt wejścia testów rdzenia (GTest::gtest_main dostarcza main()).
// Ten plik istnieje, żeby móc dodać globalne środowisko testów (np. fixture
// katalogu miejscowości) bez zmian w CMakeLists.
#include <gtest/gtest.h>

namespace {
class CoreEnvironment : public ::testing::Environment {
public:
    void SetUp() override {}
    void TearDown() override {}
};
const bool kRegistered = [] {
    ::testing::AddGlobalTestEnvironment(new CoreEnvironment());
    return true;
}();
[[maybe_unused]] const bool kUnused = kRegistered;
}  // namespace
