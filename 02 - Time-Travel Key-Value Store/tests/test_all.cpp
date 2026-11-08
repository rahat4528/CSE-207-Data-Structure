#include "MiniTest.h"

// All test cases are registered via TEST_CASE macros across the individual test translation units
int main() {
    return minitest::TestRegistry::instance().runAll();
}
