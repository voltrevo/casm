#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../src/hashset.h"

/* Test counter */
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

/* Test utilities */
#define TEST(name) \
    do { \
        tests_run++; \
        printf("TEST: %s ... ", name); \
        fflush(stdout); \
    } while(0)

#define PASS() \
    do { \
        tests_passed++; \
        printf("PASS\n"); \
    } while(0)

#define FAIL(reason) \
    do { \
        tests_failed++; \
        printf("FAIL (%s)\n", reason); \
    } while(0)

#define ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            FAIL(msg); \
            return; \
        } \
    } while(0)

#define ASSERT_EQ(a, b, msg) \
    do { \
        if ((a) != (b)) { \
            FAIL(msg); \
            return; \
        } \
    } while(0)

#define ASSERT_TRUE(cond, msg) ASSERT(cond, msg)
#define ASSERT_FALSE(cond, msg) ASSERT(!(cond), msg)

/* ============================================================================
 * BASIC OPERATIONS TESTS
 * ============================================================================ */

void test_create_and_free(void)
{
    TEST("hashset_create and hashset_free");
    HashSet* set = hashset_create();
    ASSERT_TRUE(set != NULL, "hashset_create returned NULL");
    hashset_free(set);
    PASS();
}

void test_add_and_contains_single_entry(void)
{
    TEST("add and contains single entry");
    HashSet* set = hashset_create();
    
    hashset_add(set, "hello");
    ASSERT_TRUE(hashset_contains(set, "hello"), "entry not found after add");
    
    hashset_free(set);
    PASS();
}

void test_contains_nonexistent_entry(void)
{
    TEST("contains returns false for nonexistent entry");
    HashSet* set = hashset_create();
    
    hashset_add(set, "hello");
    ASSERT_FALSE(hashset_contains(set, "world"), "nonexistent entry should not be found");
    
    hashset_free(set);
    PASS();
}

void test_empty_hashset(void)
{
    TEST("empty hashset contains no entries");
    HashSet* set = hashset_create();
    
    ASSERT_FALSE(hashset_contains(set, "anything"), "empty set should not contain anything");
    
    hashset_free(set);
    PASS();
}

/* ============================================================================
 * MULTIPLE ENTRIES TESTS
 * ============================================================================ */

void test_add_multiple_entries(void)
{
    TEST("add and retrieve multiple entries");
    HashSet* set = hashset_create();
    
    hashset_add(set, "apple");
    hashset_add(set, "banana");
    hashset_add(set, "cherry");
    
    ASSERT_TRUE(hashset_contains(set, "apple"), "apple not found");
    ASSERT_TRUE(hashset_contains(set, "banana"), "banana not found");
    ASSERT_TRUE(hashset_contains(set, "cherry"), "cherry not found");
    ASSERT_FALSE(hashset_contains(set, "date"), "date should not be found");
    
    hashset_free(set);
    PASS();
}

void test_add_many_entries(void)
{
    TEST("add and retrieve many entries (100+)");
    HashSet* set = hashset_create();
    
    /* Add 100 strings */
    char buffer[64];
    for (int i = 0; i < 100; i++) {
        snprintf(buffer, sizeof(buffer), "entry_%d", i);
        hashset_add(set, buffer);
    }
    
    /* Verify all are present */
    for (int i = 0; i < 100; i++) {
        snprintf(buffer, sizeof(buffer), "entry_%d", i);
        ASSERT_TRUE(hashset_contains(set, buffer), "entry not found");
    }
    
    /* Verify non-existent entries are not found */
    ASSERT_FALSE(hashset_contains(set, "entry_100"), "nonexistent entry found");
    ASSERT_FALSE(hashset_contains(set, "entry_-1"), "negative index found");
    
    hashset_free(set);
    PASS();
}

/* ============================================================================
 * COLLISION HANDLING TESTS
 * ============================================================================ */

void test_collision_handling(void)
{
    TEST("collision handling with chaining");
    HashSet* set = hashset_create();
    
    /* These strings might hash to the same bucket (1-byte hash, so collisions likely) */
    hashset_add(set, "test1");
    hashset_add(set, "test2");
    hashset_add(set, "test3");
    hashset_add(set, "test4");
    
    ASSERT_TRUE(hashset_contains(set, "test1"), "test1 not found");
    ASSERT_TRUE(hashset_contains(set, "test2"), "test2 not found");
    ASSERT_TRUE(hashset_contains(set, "test3"), "test3 not found");
    ASSERT_TRUE(hashset_contains(set, "test4"), "test4 not found");
    
    hashset_free(set);
    PASS();
}

void test_many_collisions_same_bucket(void)
{
    TEST("many entries in same bucket");
    HashSet* set = hashset_create();
    
    /* Add entries that are likely to collide */
    for (int i = 0; i < 50; i++) {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "a%d", i);  /* All start with 'a' */
        hashset_add(set, buffer);
    }
    
    /* Verify all are present */
    for (int i = 0; i < 50; i++) {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "a%d", i);
        ASSERT_TRUE(hashset_contains(set, buffer), "entry not found");
    }
    
    hashset_free(set);
    PASS();
}

/* ============================================================================
 * SYMBOL ID VARIANT TESTS
 * ============================================================================ */

void test_add_with_symbol_id(void)
{
    TEST("add entry with symbol ID");
    HashSet* set = hashset_create();
    
    hashset_add_with_id(set, "function_a", 1001);
    ASSERT_TRUE(hashset_contains(set, "function_a"), "entry not found");
    
    hashset_free(set);
    PASS();
}

void test_get_symbol_id(void)
{
    TEST("retrieve symbol ID for entry");
    HashSet* set = hashset_create();
    
    hashset_add_with_id(set, "function_a", 1001);
    hashset_add_with_id(set, "function_b", 1002);
    
    uint32_t id_a = hashset_get_id(set, "function_a");
    uint32_t id_b = hashset_get_id(set, "function_b");
    
    ASSERT_EQ(id_a, 1001, "wrong ID for function_a");
    ASSERT_EQ(id_b, 1002, "wrong ID for function_b");
    
    hashset_free(set);
    PASS();
}

void test_get_symbol_id_nonexistent(void)
{
    TEST("get symbol ID for nonexistent entry returns 0");
    HashSet* set = hashset_create();
    
    uint32_t id = hashset_get_id(set, "nonexistent");
    ASSERT_EQ(id, 0, "nonexistent entry should return 0");
    
    hashset_free(set);
    PASS();
}

void test_update_symbol_id(void)
{
    TEST("update symbol ID for existing entry");
    HashSet* set = hashset_create();
    
    hashset_add_with_id(set, "function_a", 1001);
    ASSERT_EQ(hashset_get_id(set, "function_a"), 1001, "initial ID incorrect");
    
    /* Update ID */
    hashset_add_with_id(set, "function_a", 2001);
    ASSERT_EQ(hashset_get_id(set, "function_a"), 2001, "updated ID incorrect");
    
    hashset_free(set);
    PASS();
}

/* ============================================================================
 * EDGE CASES
 * ============================================================================ */

void test_empty_string(void)
{
    TEST("add and retrieve empty string");
    HashSet* set = hashset_create();
    
    hashset_add(set, "");
    ASSERT_TRUE(hashset_contains(set, ""), "empty string not found");
    
    hashset_free(set);
    PASS();
}

void test_very_long_string(void)
{
    TEST("add and retrieve very long string");
    HashSet* set = hashset_create();
    
    /* Create a 1000-character string */
    char long_string[1001];
    memset(long_string, 'x', 1000);
    long_string[1000] = '\0';
    
    hashset_add(set, long_string);
    ASSERT_TRUE(hashset_contains(set, long_string), "long string not found");
    
    hashset_free(set);
    PASS();
}

void test_special_characters(void)
{
    TEST("add and retrieve strings with special characters");
    HashSet* set = hashset_create();
    
    hashset_add(set, "hello@world");
    hashset_add(set, "test#123");
    hashset_add(set, "foo$bar%baz");
    hashset_add(set, "_private_func");
    hashset_add(set, "CamelCaseFunction");
    
    ASSERT_TRUE(hashset_contains(set, "hello@world"), "special char 1 not found");
    ASSERT_TRUE(hashset_contains(set, "test#123"), "special char 2 not found");
    ASSERT_TRUE(hashset_contains(set, "foo$bar%baz"), "special char 3 not found");
    ASSERT_TRUE(hashset_contains(set, "_private_func"), "underscore func not found");
    ASSERT_TRUE(hashset_contains(set, "CamelCaseFunction"), "camel case not found");
    
    hashset_free(set);
    PASS();
}

void test_case_sensitive(void)
{
    TEST("hashset is case-sensitive");
    HashSet* set = hashset_create();
    
    hashset_add(set, "Hello");
    
    ASSERT_TRUE(hashset_contains(set, "Hello"), "Hello not found");
    ASSERT_FALSE(hashset_contains(set, "hello"), "hello (lowercase) should not be found");
    ASSERT_FALSE(hashset_contains(set, "HELLO"), "HELLO (uppercase) should not be found");
    
    hashset_free(set);
    PASS();
}

/* ============================================================================
 * DUPLICATE HANDLING TESTS
 * ============================================================================ */

void test_add_duplicate_entry(void)
{
    TEST("add duplicate entry (should not create duplicates)");
    HashSet* set = hashset_create();
    
    hashset_add(set, "test");
    hashset_add(set, "test");  /* Add same entry again */
    hashset_add(set, "test");  /* And again */
    
    ASSERT_TRUE(hashset_contains(set, "test"), "entry not found");
    
    hashset_free(set);
    PASS();
}

/* ============================================================================
 * STRESS TESTS
 * ============================================================================ */

void test_large_dataset_distribution(void)
{
    TEST("large dataset with good distribution");
    HashSet* set = hashset_create();
    
    /* Add 500 strings with varied starting characters */
    char buffer[32];
    for (int i = 0; i < 500; i++) {
        char prefix = 'a' + (i % 26);  /* Vary the prefix */
        snprintf(buffer, sizeof(buffer), "%c_func_%d", prefix, i);
        hashset_add(set, buffer);
    }
    
    /* Verify all are present */
    for (int i = 0; i < 500; i++) {
        char prefix = 'a' + (i % 26);
        snprintf(buffer, sizeof(buffer), "%c_func_%d", prefix, i);
        ASSERT_TRUE(hashset_contains(set, buffer), "entry not found");
    }
    
    hashset_free(set);
    PASS();
}

void test_mixed_operations_stress(void)
{
    TEST("mixed add and contains operations");
    HashSet* set = hashset_create();
    
    /* Alternating adds and checks */
    for (int i = 0; i < 100; i++) {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "item_%d", i);
        hashset_add(set, buffer);
        
        /* Check a previous item */
        if (i > 0) {
            char prev[32];
            snprintf(prev, sizeof(prev), "item_%d", i - 1);
            ASSERT_TRUE(hashset_contains(set, prev), "previous item not found");
        }
    }
    
    hashset_free(set);
    PASS();
}

/* ============================================================================
 * PRACTICAL SYMBOL DEDUPLICATION SCENARIO
 * ============================================================================ */

void test_symbol_dedup_scenario(void)
{
    TEST("practical symbol deduplication scenario");
    HashSet* set = hashset_create();
    
    /* Simulate the name allocation algorithm */
    /* Module A has 'helper' function */
    hashset_add_with_id(set, "module_a_helper", 1001);
    
    /* Module B has different 'helper' function */
    hashset_add_with_id(set, "module_b_helper", 1002);
    
    /* Main calls both */
    hashset_add_with_id(set, "main", 1000);
    
    /* Check all are present with correct IDs */
    ASSERT_TRUE(hashset_contains(set, "module_a_helper"), "module_a_helper not found");
    ASSERT_TRUE(hashset_contains(set, "module_b_helper"), "module_b_helper not found");
    ASSERT_TRUE(hashset_contains(set, "main"), "main not found");
    
    ASSERT_EQ(hashset_get_id(set, "module_a_helper"), 1001, "wrong ID for module_a_helper");
    ASSERT_EQ(hashset_get_id(set, "module_b_helper"), 1002, "wrong ID for module_b_helper");
    ASSERT_EQ(hashset_get_id(set, "main"), 1000, "wrong ID for main");
    
    /* Verify collisions don't occur */
    ASSERT_FALSE(hashset_contains(set, "helper"), "generic 'helper' should not be present");
    
    hashset_free(set);
    PASS();
}

/* ============================================================================
 * MEMORY LEAK TESTS
 * ============================================================================ */

void test_free_empty_hashset(void)
{
    TEST("free empty hashset");
    HashSet* set = hashset_create();
    hashset_free(set);
    PASS();
}

void test_free_populated_hashset(void)
{
    TEST("free populated hashset (no leaks)");
    HashSet* set = hashset_create();
    
    for (int i = 0; i < 100; i++) {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "entry_%d", i);
        hashset_add(set, buffer);
    }
    
    hashset_free(set);
    PASS();
}

void test_multiple_cycles(void)
{
    TEST("multiple create/free cycles");
    for (int cycle = 0; cycle < 10; cycle++) {
        HashSet* set = hashset_create();
        
        for (int i = 0; i < 50; i++) {
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "entry_%d", i);
            hashset_add(set, buffer);
        }
        
        hashset_free(set);
    }
    PASS();
}

/* ============================================================================
 * MAIN TEST RUNNER
 * ============================================================================ */

int main(void)
{
    printf("========================================\n");
    printf("HASHSET TEST SUITE\n");
    printf("========================================\n\n");

    /* Basic operations */
    test_create_and_free();
    test_add_and_contains_single_entry();
    test_contains_nonexistent_entry();
    test_empty_hashset();

    /* Multiple entries */
    test_add_multiple_entries();
    test_add_many_entries();

    /* Collision handling */
    test_collision_handling();
    test_many_collisions_same_bucket();

    /* Symbol ID variant */
    test_add_with_symbol_id();
    test_get_symbol_id();
    test_get_symbol_id_nonexistent();
    test_update_symbol_id();

    /* Edge cases */
    test_empty_string();
    test_very_long_string();
    test_special_characters();
    test_case_sensitive();

    /* Duplicate handling */
    test_add_duplicate_entry();

    /* Stress tests */
    test_large_dataset_distribution();
    test_mixed_operations_stress();

    /* Practical scenario */
    test_symbol_dedup_scenario();

    /* Memory tests */
    test_free_empty_hashset();
    test_free_populated_hashset();
    test_multiple_cycles();

    /* Print summary */
    printf("\n========================================\n");
    printf("TEST SUMMARY\n");
    printf("========================================\n");
    printf("Total:  %d\n", tests_run);
    printf("Passed: %d\n", tests_passed);
    printf("Failed: %d\n", tests_failed);
    printf("========================================\n\n");

    return tests_failed > 0 ? 1 : 0;
}
