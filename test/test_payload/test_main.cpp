#include <unity.h>
#include <math.h>
#include <string.h>
#include "payload.h"

void setUp() {}
void tearDown() {}

static void check(const Reading& r, const char* expected) {
  char buf[96];
  size_t len = formatPayload(buf, sizeof buf, r);
  TEST_ASSERT_EQUAL_STRING(expected, buf);
  TEST_ASSERT_EQUAL(strlen(expected), len);
}

void test_normal() { check({27.4f, 61.2f, 1008.3f}, "{\"temp_c\":27.4,\"humidity\":61.2,\"pressure_hpa\":1008.3}"); }
void test_negative_temp() { check({-5.5f, 40.0f, 1000.0f}, "{\"temp_c\":-5.5,\"humidity\":40.0,\"pressure_hpa\":1000.0}"); }
void test_nan_is_null() { check({20.0f, NAN, 1010.0f}, "{\"temp_c\":20.0,\"humidity\":null,\"pressure_hpa\":1010.0}"); }
void test_truncation() {
  char buf[10];
  TEST_ASSERT_EQUAL(0, formatPayload(buf, sizeof buf, {1, 2, 3}));
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_normal);
  RUN_TEST(test_negative_temp);
  RUN_TEST(test_nan_is_null);
  RUN_TEST(test_truncation);
  return UNITY_END();
}
