#include "test_helpers.h"

std::atomic_int64_t checks_checked = std::atomic_int64_t(0);
std::atomic_int64_t checks_passed = std::atomic_int64_t(0);

int main() {
    TEST_EC_Bitset();
    TEST_EC_Manager();
    TEST_EC_MoveComponentWithUniquePtr();
    TEST_EC_DeletedEntities();
    TEST_EC_FunctionStorage();
    TEST_EC_DeletedEntityID();
    TEST_EC_MultiThreaded();
    TEST_EC_ForMatchingSignatures();
    TEST_EC_forMatchingPtrs();
    TEST_EC_context();
    TEST_EC_FunctionStorageOrder();
    TEST_EC_forMatchingSimple();
    TEST_EC_forMatchingIterableFn();
    TEST_EC_MultiThreadedForMatching();
    TEST_EC_ManagerWithLowThreadCount();
    TEST_EC_ManagerDeferredDeletions();
    TEST_EC_NestedThreadPoolTasks();

    TEST_Meta_Contains();
    TEST_Meta_ContainsAll();
    TEST_Meta_IndexOf();
    TEST_Meta_Bitset();
    TEST_Meta_Combine();
    TEST_Meta_Morph();
    TEST_Meta_TypeListGet();
    TEST_Meta_ForEach();
    TEST_Meta_Matching();

    TEST_ECThreadPool_OneThread();
    TEST_ECThreadPool_Simple();
    TEST_ECThreadPool_QueryCount();
    TEST_ECThreadPool_easyStartAndWait();

    std::cout << "checks_checked: " << checks_checked.load() << '\n'
              << "checks_passed:  " << checks_passed.load() << std::endl;

    return checks_checked.load() == checks_passed.load() ? 0 : 1;
}
