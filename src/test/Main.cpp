#include "test_helpers.h"

int checks_checked = 0;
int checks_passed = 0;

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

    std::cout << "checks_checked: " << checks_checked << '\n'
              << "checks_passed:  " << checks_passed << std::endl;

    return checks_checked == checks_passed ? 0 : 1;
}
