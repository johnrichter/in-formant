#include <gmock/gmock.h>
#include <gtest/gtest.h>

// These tests check the framework itself: that GoogleTest runs a test, that GoogleMock links and checks
// a call, and that the build uses C++17. A failure here means that the setup of the tests is wrong.

namespace {

class Counter {
public:
    virtual ~Counter() = default;
    virtual int next() = 0;
};

class MockCounter : public Counter {
public:
    MOCK_METHOD(int, next, (), (override));
};

}

TEST(Framework, RunsATest)
{
    EXPECT_EQ(2 + 2, 4);
}

TEST(Framework, MockChecksACall)
{
    using ::testing::Return;
    MockCounter counter;
    EXPECT_CALL(counter, next()).Times(2).WillOnce(Return(1)).WillOnce(Return(2));
    EXPECT_EQ(counter.next(), 1);
    EXPECT_EQ(counter.next(), 2);
}

TEST(Framework, UsesCpp17)
{
    // An if statement with an initializer needs C++17.
    if (const int value = 3; value > 2) {
        SUCCEED();
    }
    else {
        FAIL();
    }
    static_assert(__cplusplus >= 201703L, "The tests need C++17");
}
