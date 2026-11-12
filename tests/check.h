#ifndef TESTS_CHECK_H
#define TESTS_CHECK_H

#include <cmath>
#include <cstdio>

// A small check macro for the tests. A failed check prints the line and sets the failure count.
static int gFailures = 0;

#define CHECK(condition)                                                                 \
    do {                                                                                 \
        if (!(condition)) {                                                              \
            std::printf("%s:%d: check failed: %s\n", __FILE__, __LINE__, #condition);    \
            ++gFailures;                                                                 \
        }                                                                                \
    } while (0)

#define CHECK_NEAR(a, b, tolerance) CHECK(std::fabs((a) - (b)) <= (tolerance))

#define TEST_RESULT() (gFailures == 0 ? 0 : 1)

#endif // TESTS_CHECK_H
