/* This is auto-generated code. Edit at your own peril. */
#include "tests/gtest-helpers.h"
#include "run-auto-test.h"

static void start_tests (void) {
}

static void stop_tests (void) {
}

static void initialize_tests (void) {
	g_test_add("/test-dh/dh_perform", int, NULL, NULL, test_dh_perform, NULL);
	g_test_add("/test-dh/dh_short_pair", int, NULL, NULL, test_dh_short_pair, NULL);
	g_test_add("/test-dh/dh_default_768", int, NULL, NULL, test_dh_default_768, NULL);
	g_test_add("/test-dh/dh_default_1024", int, NULL, NULL, test_dh_default_1024, NULL);
	g_test_add("/test-dh/dh_default_1536", int, NULL, NULL, test_dh_default_1536, NULL);
	g_test_add("/test-dh/dh_default_2048", int, NULL, NULL, test_dh_default_2048, NULL);
	g_test_add("/test-dh/dh_default_3072", int, NULL, NULL, test_dh_default_3072, NULL);
	g_test_add("/test-dh/dh_default_4096", int, NULL, NULL, test_dh_default_4096, NULL);
	g_test_add("/test-dh/dh_default_8192", int, NULL, NULL, test_dh_default_8192, NULL);
	g_test_add("/test-dh/dh_default_bad", int, NULL, NULL, test_dh_default_bad, NULL);
	g_test_add("/secmem/secmem_alloc_free", int, NULL, NULL, test_secmem_alloc_free, NULL);
	g_test_add("/secmem/secmem_realloc_across", int, NULL, NULL, test_secmem_realloc_across, NULL);
	g_test_add("/secmem/secmem_alloc_two", int, NULL, NULL, test_secmem_alloc_two, NULL);
	g_test_add("/secmem/secmem_realloc", int, NULL, NULL, test_secmem_realloc, NULL);
	g_test_add("/secmem/secmem_multialloc", int, NULL, NULL, test_secmem_multialloc, NULL);
	g_test_add("/secmem/secmem_clear", int, NULL, NULL, test_secmem_clear, NULL);
	g_test_add("/secmem/secmem_strclear", int, NULL, NULL, test_secmem_strclear, NULL);
}

#include "tests/gtest-helpers.c"
