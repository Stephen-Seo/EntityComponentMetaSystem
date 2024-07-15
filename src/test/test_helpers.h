#ifndef SEODISPARATE_COM_ENTITY_COMPONENT_META_SYSTEM_TEST_HELPERS_H_
#define SEODISPARATE_COM_ENTITY_COMPONENT_META_SYSTEM_TEST_HELPERS_H_

#include <cstring>
#include <iostream>
#include <atomic>

extern std::atomic_int64_t checks_checked;
extern std::atomic_int64_t checks_passed;

// Macros for unit testing.

#define CHECK_TRUE(x)                                                     \
  do {                                                                    \
    checks_checked.fetch_add(1);                                                     \
    if (!(x)) {                                                           \
      std::cout << "CHECK_TRUE at line " << __LINE__ << " failed: " << #x \
                << '\n';                                                  \
    } else {                                                              \
      checks_passed.fetch_add(1);                                                    \
    }                                                                     \
  } while (false);
#define ASSERT_TRUE(x)                                                    \
  do {                                                                    \
    checks_checked.fetch_add(1);                                                     \
    if (!(x)) {                                                           \
      std::cout << "CHECK_TRUE at line " << __LINE__ << " failed: " << #x \
                << '\n';                                                  \
      return;                                                             \
    } else {                                                              \
      checks_passed.fetch_add(1);                                                    \
    }                                                                     \
  } while (false);
#define CHECK_FALSE(x)                                                     \
  do {                                                                     \
    checks_checked.fetch_add(1);                                                      \
    if (x) {                                                               \
      std::cout << "CHECK_FALSE at line " << __LINE__ << " failed: " << #x \
                << '\n';                                                   \
    } else {                                                               \
      checks_passed.fetch_add(1);                                                     \
    }                                                                      \
  } while (false);
#define ASSERT_FALSE(x)                                                    \
  do {                                                                     \
    checks_checked.fetch_add(1);                                                      \
    if (x) {                                                               \
      std::cout << "CHECK_FALSE at line " << __LINE__ << " failed: " << #x \
                << '\n';                                                   \
      return;                                                              \
    } else {                                                               \
      checks_passed.fetch_add(1);                                                     \
    }                                                                      \
  } while (false);

#define CHECK_FLOAT(var, value)                                              \
  do {                                                                       \
    checks_checked.fetch_add(1);                                                        \
    if ((var) > (value) - 0.0001F && (var) < (value) + 0.0001F) {            \
      checks_passed.fetch_add(1);                                                       \
    } else {                                                                 \
      std::cout << "CHECK_FLOAT at line " << __LINE__ << " failed: " << #var \
                << " != " << #value << '\n';                                 \
    }                                                                        \
  } while (false);

#define CHECK_EQ(var, value)                                              \
  do {                                                                    \
    checks_checked.fetch_add(1);                                                     \
    if ((var) == (value)) {                                               \
      checks_passed.fetch_add(1);                                                    \
    } else {                                                              \
      std::cout << "CHECK_EQ at line " << __LINE__ << " failed: " << #var \
                << " != " << #value << '\n';                              \
    }                                                                     \
  } while (false);
#define ASSERT_EQ(var, value)                                              \
  do {                                                                     \
    checks_checked.fetch_add(1);                                                      \
    if ((var) == (value)) {                                                \
      checks_passed.fetch_add(1);                                                     \
    } else {                                                               \
      std::cout << "ASSERT_EQ at line " << __LINE__ << " failed: " << #var \
                << " != " << #value << '\n';                               \
      return;                                                              \
    }                                                                      \
  } while (false);
#define CHECK_NE(var, value)                                              \
  do {                                                                    \
    checks_checked.fetch_add(1);                                                     \
    if ((var) != (value)) {                                               \
      checks_passed.fetch_add(1);                                                    \
    } else {                                                              \
      std::cout << "CHECK_NE at line " << __LINE__ << " failed: " << #var \
                << " == " << #value << '\n';                              \
    }                                                                     \
  } while (false);
#define CHECK_GE(var, value)                                              \
  do {                                                                    \
    checks_checked.fetch_add(1);                                                     \
    if ((var) >= (value)) {                                               \
      checks_passed.fetch_add(1);                                                    \
    } else {                                                              \
      std::cout << "CHECK_GE at line " << __LINE__ << " failed: " << #var \
                << " < " << #value << '\n';                               \
    }                                                                     \
  } while (false);
#define CHECK_LE(var, value)                                              \
  do {                                                                    \
    checks_checked.fetch_add(1);                                                     \
    if ((var) <= (value)) {                                               \
      checks_passed.fetch_add(1);                                                    \
    } else {                                                              \
      std::cout << "CHECK_LE at line " << __LINE__ << " failed: " << #var \
                << " > " << #value << '\n';                               \
    }                                                                     \
  } while (false);

#define CHECK_STREQ(str_a, str_b)                                             \
  do {                                                                        \
    checks_checked.fetch_add(1);                                                         \
    if (std::strcmp((str_a), (str_b)) == 0) {                                 \
      checks_passed.fetch_add(1);                                                        \
    } else {                                                                  \
      std::cout << "CHECK_STREQ at line " << __LINE__ << "failed: " << #str_a \
                << " != " << #str_b << '\n';                                  \
    }                                                                         \
  } while (false);

// Tests.

void TEST_EC_Bitset();
void TEST_EC_Manager();
void TEST_EC_MoveComponentWithUniquePtr();
void TEST_EC_DeletedEntities();
void TEST_EC_FunctionStorage();
void TEST_EC_DeletedEntityID();
void TEST_EC_MultiThreaded();
void TEST_EC_ForMatchingSignatures();
void TEST_EC_forMatchingPtrs();
void TEST_EC_context();
void TEST_EC_FunctionStorageOrder();
void TEST_EC_forMatchingSimple();
void TEST_EC_forMatchingIterableFn();
void TEST_EC_MultiThreadedForMatching();
void TEST_EC_ManagerWithLowThreadCount();
void TEST_EC_ManagerDeferredDeletions();
void TEST_EC_NestedThreadPoolTasks();

void TEST_Meta_Contains();
void TEST_Meta_ContainsAll();
void TEST_Meta_IndexOf();
void TEST_Meta_Bitset();
void TEST_Meta_Combine();
void TEST_Meta_Morph();
void TEST_Meta_TypeListGet();
void TEST_Meta_ForEach();
void TEST_Meta_Matching();

void TEST_ECThreadPool_OneThread();
void TEST_ECThreadPool_Simple();
void TEST_ECThreadPool_QueryCount();
void TEST_ECThreadPool_easyStartAndWait();
#endif
