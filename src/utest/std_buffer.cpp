/* Copyright (c) 2017 Nguyen Viet Giang. All rights reserved. */
#include <any/platform.h>
#include <catch.hpp>

#include <any/rt_types.h>
#include <any/asm.h>
#include <any/gc.h>
#include <any/scheduler.h>
#include <any/loader.h>
#include <any/actor.h>
#include <any/std_buffer.h>
#include <any/std_string.h>

enum { CSTACK_SZ = 16384*2 };

static void* myalloc(void*, void* old, aint_t sz)
{
    return realloc(old, (size_t)sz);
}

static void fill(const void* buf, aint_t sz)
{
    uint8_t* b = (uint8_t*)buf;
    for (aint_t i = 0; i < sz; ++i) {
        b[i] = (uint8_t)(i % 255);
    }
}

static void test(const void* buf, aint_t sz)
{
    uint8_t* b = (uint8_t*)buf;
    for (aint_t i = 0; i < sz; ++i) {
        REQUIRE(b[i] == (uint8_t)(i % 255));
    }
}

static void buffer_test(aactor_t* a)
{
    any_push_buffer(a, 8);

    aint_t b_idx = any_check_index(a, 0);

    REQUIRE(any_buffer_size(a, b_idx) == 0);
    REQUIRE(any_buffer_capacity(a, b_idx) == 8);

    for (aint_t i = 1; i <= 100; ++i) {
        aint_t new_cap = i * 10;
        any_buffer_reserve(a, b_idx, new_cap);
        REQUIRE(any_buffer_capacity(a, b_idx) == new_cap);
    }

    any_buffer_shrink_to_fit(a, b_idx);
    REQUIRE(any_buffer_size(a, b_idx) == 0);
    REQUIRE(any_buffer_capacity(a, b_idx) == 0);

    for (aint_t i = 1; i <= 100; ++i) {
        aint_t new_sz = i * 10;
        test(any_check_buffer(a, b_idx), any_buffer_size(a, b_idx));
        any_buffer_resize(a, b_idx, new_sz);
        REQUIRE(any_buffer_capacity(a, b_idx) >= new_sz);
        REQUIRE(any_buffer_size(a, b_idx) == new_sz);
        fill(any_check_buffer(a, b_idx), new_sz);
    }

    any_buffer_shrink_to_fit(a, b_idx);
    REQUIRE(any_buffer_size(a, b_idx) == 1000);
    REQUIRE(any_buffer_capacity(a, b_idx) == 1000);
    test(any_check_buffer(a, b_idx), any_buffer_size(a, b_idx));

    any_push_integer(a, 0xFFEE);
}

static void lib_fill(aactor_t* a, aint_t sz)
{
    aint_t b_idx = any_check_index(a, 0);
    for (aint_t i = 0; i < sz; ++i) {
        any_find(a, "std-buffer", "set/3");
        any_push_integer(a, i % 255);
        any_push_integer(a, i);
        any_push_index(a, b_idx);
        any_call(a, 3);
        REQUIRE(any_type(a, b_idx + 1).type == AVT_NIL);
        any_pop(a, 1);
    }
}

static void lib_test(aactor_t* a, aint_t sz)
{
    aint_t b_idx = any_check_index(a, 0);
    for (aint_t i = 0; i < sz; ++i) {
        any_find(a, "std-buffer", "get/2");
        any_push_integer(a, i);
        any_push_index(a, b_idx);
        any_call(a, 2);
        REQUIRE(any_check_integer(a, b_idx + 1) == i % 255);
        any_pop(a, 1);
    }
}

static void buffer_binding_test(aactor_t* a)
{
    aint_t buf_sz;

    any_find(a, "std-buffer", "new/1");
    any_push_integer(a, 8);
    any_call(a, 1);

    aint_t b_idx = any_check_index(a, 0);

    any_find(a, "std-buffer", "size/1");
    any_push_index(a, b_idx);
    any_call(a, 1);
    REQUIRE(any_check_integer(a, b_idx + 1) == 0);
    any_pop(a, 1);

    any_find(a, "std-buffer", "capacity/1");
    any_push_index(a, b_idx);
    any_call(a, 1);
    REQUIRE(any_check_integer(a, b_idx + 1) == 8);
    any_pop(a, 1);

    for (aint_t i = 1; i <= 100; ++i) {
        aint_t new_cap = i * 10;

        any_find(a, "std-buffer", "reserve/2");
        any_push_integer(a, new_cap);
        any_push_index(a, b_idx);
        any_call(a, 2);
        REQUIRE(any_type(a, b_idx + 1).type == AVT_NIL);
        any_pop(a, 1);

        any_find(a, "std-buffer", "capacity/1");
        any_push_index(a, b_idx);
        any_call(a, 1);
        REQUIRE(any_check_integer(a, b_idx + 1) == new_cap);
        any_pop(a, 1);
    }

    any_find(a, "std-buffer", "shrink_to_fit/1");
    any_push_index(a, b_idx);
    any_call(a, 1);
    REQUIRE(any_type(a, b_idx + 1).type == AVT_NIL);
    any_pop(a, 1);

    any_find(a, "std-buffer", "size/1");
    any_push_index(a, b_idx);
    any_call(a, 1);
    REQUIRE(any_check_integer(a, b_idx + 1) == 0);
    any_pop(a, 1);

    any_find(a, "std-buffer", "capacity/1");
    any_push_index(a, b_idx);
    any_call(a, 1);
    REQUIRE(any_check_integer(a, b_idx + 1) == 0);
    any_pop(a, 1);

    for (aint_t i = 1; i <= 100; ++i) {
        aint_t new_sz = i * 10;

        any_find(a, "std-buffer", "size/1");
        any_push_index(a, b_idx);
        any_call(a, 1);
        buf_sz = any_check_integer(a, b_idx + 1);
        any_pop(a, 1);
        lib_test(a, buf_sz);

        any_find(a, "std-buffer", "resize/2");
        any_push_integer(a, new_sz);
        any_push_index(a, b_idx);
        any_call(a, 2);
        REQUIRE(any_type(a, b_idx + 1).type == AVT_NIL);
        any_pop(a, 1);

        any_buffer_resize(a, b_idx, new_sz);

        any_find(a, "std-buffer", "capacity/1");
        any_push_index(a, b_idx);
        any_call(a, 1);
        REQUIRE(any_check_integer(a, b_idx + 1) >= new_sz);
        any_pop(a, 1);

        any_find(a, "std-buffer", "size/1");
        any_push_index(a, b_idx);
        any_call(a, 1);
        REQUIRE(any_check_integer(a, b_idx + 1) == new_sz);
        any_pop(a, 1);

        any_find(a, "std-buffer", "size/1");
        any_push_index(a, b_idx);
        any_call(a, 1);
        buf_sz = any_check_integer(a, b_idx + 1);
        any_pop(a, 1);
        lib_fill(a, buf_sz);
    }

    any_find(a, "std-buffer", "shrink_to_fit/1");
    any_push_index(a, b_idx);
    any_call(a, 1);
    REQUIRE(any_type(a, b_idx + 1).type == AVT_NIL);
    any_pop(a, 1);

    any_find(a, "std-buffer", "size/1");
    any_push_index(a, b_idx);
    any_call(a, 1);
    REQUIRE(any_check_integer(a, b_idx + 1) == 1000);
    any_pop(a, 1);

    any_find(a, "std-buffer", "capacity/1");
    any_push_index(a, b_idx);
    any_call(a, 1);
    REQUIRE(any_check_integer(a, b_idx + 1) == 1000);
    any_pop(a, 1);

    any_find(a, "std-buffer", "size/1");
    any_push_index(a, b_idx);
    any_call(a, 1);
    buf_sz = any_check_integer(a, b_idx + 1);
    any_pop(a, 1);
    lib_fill(a, buf_sz);

    any_push_integer(a, 0xFFFA);
}

static void add_test_module(aasm_t* a)
{
    aasm_prototype_t* p = aasm_prototype(a);
    p->symbol = aasm_string_to_ref(a, "mod_test");
}

TEST_CASE("std_buffer")
{
    enum { NUM_IDX_BITS = 4 };
    enum { NUM_GEN_BITS = 4 };

    ascheduler_t s;

    REQUIRE(AERR_NONE ==
        ascheduler_init(&s, NUM_IDX_BITS, NUM_GEN_BITS, &myalloc, NULL));

    aactor_t* a;
    REQUIRE(AERR_NONE == ascheduler_new_actor(&s, CSTACK_SZ, &a));
    any_push_native_func(a, &buffer_test);
    ascheduler_start(&s, a, 0);

    ascheduler_run_once(&s);

    REQUIRE(any_count(a) == 2);
    REQUIRE(any_type(a, any_check_index(a, 1)).type == AVT_NIL);
    REQUIRE(any_check_integer(a, any_check_index(a, 0)) == 0xFFEE);

    ascheduler_cleanup(&s);
}

TEST_CASE("std_buffer_binding")
{
    enum { NUM_IDX_BITS = 4 };
    enum { NUM_GEN_BITS = 4 };

    ascheduler_t s;

    REQUIRE(AERR_NONE ==
        ascheduler_init(&s, NUM_IDX_BITS, NUM_GEN_BITS, &myalloc, NULL));

    astd_lib_add_buffer(&s.loader);

    aactor_t* a;
    REQUIRE(AERR_NONE == ascheduler_new_actor(&s, CSTACK_SZ, &a));
    any_push_native_func(a, &buffer_binding_test);
    ascheduler_start(&s, a, 0);

    ascheduler_run_once(&s);

    REQUIRE(any_count(a) == 2);
    REQUIRE(any_type(a, any_check_index(a, 1)).type == AVT_NIL);
    REQUIRE(any_check_integer(a, any_check_index(a, 0)) == 0xFFFA);

    ascheduler_cleanup(&s);
}

TEST_CASE("std_buffer_binding_new")
{
    enum { NUM_IDX_BITS = 4 };
    enum { NUM_GEN_BITS = 4 };

    aasm_t as;
    aasm_init(&as, &myalloc, NULL);
    REQUIRE(aasm_load(&as, NULL) == AERR_NONE);
    add_test_module(&as);

    ascheduler_t s;

    REQUIRE(AERR_NONE ==
        ascheduler_init(&s, NUM_IDX_BITS, NUM_GEN_BITS, &myalloc, NULL));

    astd_lib_add_buffer(&s.loader);

    SECTION("bad_cap")
    {
        aasm_module_push(&as, "test_f");
        aint_t lnew = aasm_add_import(&as, "std-buffer", "new/1");

        aasm_emit(&as, ai_imp(lnew));
        aasm_emit(&as, ai_lsi(-1));
        aasm_emit(&as, ai_ivk(1));

        aasm_emit(&as, ai_ret());
        aasm_pop(&as);
        aasm_save(&as);

        REQUIRE(AERR_NONE ==
            aloader_add_chunk(&s.loader, as.chunk, as.chunk_size, NULL, NULL));
        REQUIRE(AERR_NONE == aloader_link(&s.loader, TRUE));

        aactor_t* a;
        REQUIRE(AERR_NONE == ascheduler_new_actor(&s, CSTACK_SZ, &a));
        any_find(a, "mod_test", "test_f");
        ascheduler_start(&s, a, 0);

        ascheduler_run_once(&s);

        REQUIRE(any_count(a) == 1);
        CHECK_THAT(any_check_string(a, any_check_index(a, 0)),
            Catch::Equals("bad capacity -1"));
    }

    ascheduler_cleanup(&s);
    aasm_cleanup(&as);
}

TEST_CASE("std_buffer_binding_reserve")
{
    enum { NUM_IDX_BITS = 4 };
    enum { NUM_GEN_BITS = 4 };

    aasm_t as;
    aasm_init(&as, &myalloc, NULL);
    REQUIRE(aasm_load(&as, NULL) == AERR_NONE);
    add_test_module(&as);

    ascheduler_t s;

    REQUIRE(AERR_NONE ==
        ascheduler_init(&s, NUM_IDX_BITS, NUM_GEN_BITS, &myalloc, NULL));

    astd_lib_add_buffer(&s.loader);

    SECTION("not_a_buffer")
    {
        aasm_module_push(&as, "test_f");
        aint_t lreserve = aasm_add_import(&as, "std-buffer", "reserve/2");

        aasm_emit(&as, ai_imp(lreserve));
        aasm_emit(&as, ai_lsi(0));
        aasm_emit(&as, ai_nil());
        aasm_emit(&as, ai_ivk(2));

        aasm_emit(&as, ai_ret());
        aasm_pop(&as);
        aasm_save(&as);

        REQUIRE(AERR_NONE ==
            aloader_add_chunk(&s.loader, as.chunk, as.chunk_size, NULL, NULL));
        REQUIRE(AERR_NONE == aloader_link(&s.loader, TRUE));

        aactor_t* a;
        REQUIRE(AERR_NONE == ascheduler_new_actor(&s, CSTACK_SZ, &a));
        any_find(a, "mod_test", "test_f");
        ascheduler_start(&s, a, 0);

        ascheduler_run_once(&s);

        REQUIRE(any_count(a) == 1);
        CHECK_THAT(any_check_string(a, any_check_index(a, 0)),
            Catch::Equals("not a buffer"));
    }

    SECTION("cap_not_a_integer")
    {
        aasm_module_push(&as, "test_f");
        aint_t lnew = aasm_add_import(&as, "std-buffer", "new/1");
        aint_t lreserve = aasm_add_import(&as, "std-buffer", "reserve/2");

        aasm_emit(&as, ai_imp(lnew));
        aasm_emit(&as, ai_lsi(8));
        aasm_emit(&as, ai_ivk(1));

        aasm_emit(&as, ai_imp(lreserve));
        aasm_emit(&as, ai_nil());
        aasm_emit(&as, ai_llv(0));
        aasm_emit(&as, ai_ivk(2));

        aasm_emit(&as, ai_ret());
        aasm_pop(&as);
        aasm_save(&as);

        REQUIRE(AERR_NONE ==
            aloader_add_chunk(&s.loader, as.chunk, as.chunk_size, NULL, NULL));
        REQUIRE(AERR_NONE == aloader_link(&s.loader, TRUE));

        aactor_t* a;
        REQUIRE(AERR_NONE == ascheduler_new_actor(&s, CSTACK_SZ, &a));
        any_find(a, "mod_test", "test_f");
        ascheduler_start(&s, a, 0);

        ascheduler_run_once(&s);

        REQUIRE(any_count(a) == 1);
        CHECK_THAT(any_check_string(a, any_check_index(a, 0)),
            Catch::Equals("not a integer"));
    }

    SECTION("negative_cap")
    {
        aasm_module_push(&as, "test_f");
        aint_t lnew = aasm_add_import(&as, "std-buffer", "new/1");
        aint_t lreserve = aasm_add_import(&as, "std-buffer", "reserve/2");
        aint_t lcapacity = aasm_add_import(&as, "std-buffer", "capacity/1");

        aasm_emit(&as, ai_imp(lnew));
        aasm_emit(&as, ai_lsi(8));
        aasm_emit(&as, ai_ivk(1));

        aasm_emit(&as, ai_imp(lreserve));
        aasm_emit(&as, ai_lsi(-1));
        aasm_emit(&as, ai_llv(0));
        aasm_emit(&as, ai_ivk(2));
        aasm_emit(&as, ai_pop(1));

        aasm_emit(&as, ai_imp(lcapacity));
        aasm_emit(&as, ai_llv(0));
        aasm_emit(&as, ai_ivk(1));

        aasm_emit(&as, ai_ret());
        aasm_pop(&as);
        aasm_save(&as);

        REQUIRE(AERR_NONE ==
            aloader_add_chunk(&s.loader, as.chunk, as.chunk_size, NULL, NULL));
        REQUIRE(AERR_NONE == aloader_link(&s.loader, TRUE));

        aactor_t* a;
        REQUIRE(AERR_NONE == ascheduler_new_actor(&s, CSTACK_SZ, &a));
        any_find(a, "mod_test", "test_f");
        ascheduler_start(&s, a, 0);

        ascheduler_run_once(&s);

        REQUIRE(any_count(a) == 2);
        REQUIRE(any_type(a, any_check_index(a, 1)).type == AVT_NIL);
        REQUIRE(any_check_integer(a, any_check_index(a, 0)) == 8);
    }

    ascheduler_cleanup(&s);
    aasm_cleanup(&as);
}

TEST_CASE("std_buffer_binding_shrink")
{
    enum { NUM_IDX_BITS = 4 };
    enum { NUM_GEN_BITS = 4 };

    aasm_t as;
    aasm_init(&as, &myalloc, NULL);
    REQUIRE(aasm_load(&as, NULL) == AERR_NONE);
    add_test_module(&as);

    ascheduler_t s;

    REQUIRE(AERR_NONE ==
        ascheduler_init(&s, NUM_IDX_BITS, NUM_GEN_BITS, &myalloc, NULL));

    astd_lib_add_buffer(&s.loader);

    SECTION("not_a_buffer")
    {
        aasm_module_push(&as, "test_f");
        aint_t lshrink = aasm_add_import(&as, "std-buffer", "shrink_to_fit/1");

        aasm_emit(&as, ai_imp(lshrink));
        aasm_emit(&as, ai_nil());
        aasm_emit(&as, ai_ivk(1));

        aasm_emit(&as, ai_ret());
        aasm_pop(&as);
        aasm_save(&as);

        REQUIRE(AERR_NONE ==
            aloader_add_chunk(&s.loader, as.chunk, as.chunk_size, NULL, NULL));
        REQUIRE(AERR_NONE == aloader_link(&s.loader, TRUE));

        aactor_t* a;
        REQUIRE(AERR_NONE == ascheduler_new_actor(&s, CSTACK_SZ, &a));
        any_find(a, "mod_test", "test_f");
        ascheduler_start(&s, a, 0);

        ascheduler_run_once(&s);

        REQUIRE(any_count(a) == 1);
        CHECK_THAT(any_check_string(a, any_check_index(a, 0)),
            Catch::Equals("not a buffer"));
    }

    ascheduler_cleanup(&s);
    aasm_cleanup(&as);
}

TEST_CASE("std_buffer_binding_resize")
{
    enum { NUM_IDX_BITS = 4 };
    enum { NUM_GEN_BITS = 4 };

    aasm_t as;
    aasm_init(&as, &myalloc, NULL);
    REQUIRE(aasm_load(&as, NULL) == AERR_NONE);
    add_test_module(&as);

    ascheduler_t s;

    REQUIRE(AERR_NONE ==
        ascheduler_init(&s, NUM_IDX_BITS, NUM_GEN_BITS, &myalloc, NULL));

    astd_lib_add_buffer(&s.loader);

    SECTION("not_a_buffer")
    {
        aasm_module_push(&as, "test_f");
        aint_t lresize = aasm_add_import(&as, "std-buffer", "resize/2");

        aasm_emit(&as, ai_imp(lresize));
        aasm_emit(&as, ai_lsi(0));
        aasm_emit(&as, ai_nil());
        aasm_emit(&as, ai_ivk(2));

        aasm_emit(&as, ai_ret());
        aasm_pop(&as);
        aasm_save(&as);

        REQUIRE(AERR_NONE ==
            aloader_add_chunk(&s.loader, as.chunk, as.chunk_size, NULL, NULL));
        REQUIRE(AERR_NONE == aloader_link(&s.loader, TRUE));

        aactor_t* a;
        REQUIRE(AERR_NONE == ascheduler_new_actor(&s, CSTACK_SZ, &a));
        any_find(a, "mod_test", "test_f");
        ascheduler_start(&s, a, 0);

        ascheduler_run_once(&s);

        REQUIRE(any_count(a) == 1);
        CHECK_THAT(any_check_string(a, any_check_index(a, 0)),
            Catch::Equals("not a buffer"));
    }

    SECTION("sz_not_a_integer")
    {
        aasm_module_push(&as, "test_f");
        aint_t lnew = aasm_add_import(&as, "std-buffer", "new/1");
        aint_t lresize = aasm_add_import(&as, "std-buffer", "resize/2");

        aasm_emit(&as, ai_imp(lnew));
        aasm_emit(&as, ai_lsi(8));
        aasm_emit(&as, ai_ivk(1));

        aasm_emit(&as, ai_imp(lresize));
        aasm_emit(&as, ai_nil());
        aasm_emit(&as, ai_llv(0));
        aasm_emit(&as, ai_ivk(2));

        aasm_emit(&as, ai_ret());
        aasm_pop(&as);
        aasm_save(&as);

        REQUIRE(AERR_NONE ==
            aloader_add_chunk(&s.loader, as.chunk, as.chunk_size, NULL, NULL));
        REQUIRE(AERR_NONE == aloader_link(&s.loader, TRUE));

        aactor_t* a;
        REQUIRE(AERR_NONE == ascheduler_new_actor(&s, CSTACK_SZ, &a));
        any_find(a, "mod_test", "test_f");
        ascheduler_start(&s, a, 0);

        ascheduler_run_once(&s);

        REQUIRE(any_count(a) == 1);
        CHECK_THAT(any_check_string(a, any_check_index(a, 0)),
            Catch::Equals("not a integer"));
    }

    SECTION("negative_sz")
    {
        aasm_module_push(&as, "test_f");
        aint_t lnew = aasm_add_import(&as, "std-buffer", "new/1");
        aint_t lresize = aasm_add_import(&as, "std-buffer", "resize/2");

        aasm_emit(&as, ai_imp(lnew));
        aasm_emit(&as, ai_lsi(8));
        aasm_emit(&as, ai_ivk(1));

        aasm_emit(&as, ai_imp(lresize));
        aasm_emit(&as, ai_lsi(-1));
        aasm_emit(&as, ai_llv(0));
        aasm_emit(&as, ai_ivk(2));
        aasm_emit(&as, ai_pop(1));

        aasm_emit(&as, ai_ret());
        aasm_pop(&as);
        aasm_save(&as);

        REQUIRE(AERR_NONE ==
            aloader_add_chunk(&s.loader, as.chunk, as.chunk_size, NULL, NULL));
        REQUIRE(AERR_NONE == aloader_link(&s.loader, TRUE));

        aactor_t* a;
        REQUIRE(AERR_NONE == ascheduler_new_actor(&s, CSTACK_SZ, &a));
        any_find(a, "mod_test", "test_f");
        ascheduler_start(&s, a, 0);

        ascheduler_run_once(&s);

        CHECK_THAT(any_check_string(a, any_check_index(a, 0)),
            Catch::Equals("bad size -1"));
    }

    ascheduler_cleanup(&s);
    aasm_cleanup(&as);
}

TEST_CASE("std_buffer_binding_get")
{
    enum { NUM_IDX_BITS = 4 };
    enum { NUM_GEN_BITS = 4 };

    aasm_t as;
    aasm_init(&as, &myalloc, NULL);
    REQUIRE(aasm_load(&as, NULL) == AERR_NONE);
    add_test_module(&as);

    ascheduler_t s;

    REQUIRE(AERR_NONE ==
        ascheduler_init(&s, NUM_IDX_BITS, NUM_GEN_BITS, &myalloc, NULL));

    astd_lib_add_buffer(&s.loader);

    SECTION("normal")
    {
        aasm_module_push(&as, "test_f");
        aint_t lnew = aasm_add_import(&as, "std-buffer", "new/1");
        aint_t lget = aasm_add_import(&as, "std-buffer", "get/2");
        aint_t lresize = aasm_add_import(&as, "std-buffer", "resize/2");

        aasm_emit(&as, ai_imp(lnew));
        aasm_emit(&as, ai_lsi(8));
        aasm_emit(&as, ai_ivk(1));

        aasm_emit(&as, ai_imp(lresize));
        aasm_emit(&as, ai_lsi(1));
        aasm_emit(&as, ai_llv(0));
        aasm_emit(&as, ai_ivk(2));
        aasm_emit(&as, ai_pop(1));

        aasm_emit(&as, ai_imp(lget));
        aasm_emit(&as, ai_lsi(0));
        aasm_emit(&as, ai_llv(0));
        aasm_emit(&as, ai_ivk(2));

        aasm_emit(&as, ai_ret());
        aasm_pop(&as);
        aasm_save(&as);

        REQUIRE(AERR_NONE ==
            aloader_add_chunk(&s.loader, as.chunk, as.chunk_size, NULL, NULL));
        REQUIRE(AERR_NONE == aloader_link(&s.loader, TRUE));

        aactor_t* a;
        REQUIRE(AERR_NONE == ascheduler_new_actor(&s, CSTACK_SZ, &a));
        any_find(a, "mod_test", "test_f");
        ascheduler_start(&s, a, 0);

        ascheduler_run_once(&s);

        REQUIRE(any_count(a) == 2);
        REQUIRE(any_type(a, any_check_index(a, 1)).type == AVT_NIL);
        REQUIRE(any_type(a, any_check_index(a, 0)).type == AVT_INTEGER);
    }

    SECTION("bad_no_elements")
    {
        aasm_module_push(&as, "test_f");
        aint_t lnew = aasm_add_import(&as, "std-buffer", "new/1");
        aint_t lget = aasm_add_import(&as, "std-buffer", "get/2");

        aasm_emit(&as, ai_imp(lnew));
        aasm_emit(&as, ai_lsi(8));
        aasm_emit(&as, ai_ivk(1));

        aasm_emit(&as, ai_imp(lget));
        aasm_emit(&as, ai_lsi(0));
        aasm_emit(&as, ai_llv(0));
        aasm_emit(&as, ai_ivk(2));

        aasm_emit(&as, ai_ret());
        aasm_pop(&as);
        aasm_save(&as);

        REQUIRE(AERR_NONE ==
            aloader_add_chunk(&s.loader, as.chunk, as.chunk_size, NULL, NULL));
        REQUIRE(AERR_NONE == aloader_link(&s.loader, TRUE));

        aactor_t* a;
        REQUIRE(AERR_NONE == ascheduler_new_actor(&s, CSTACK_SZ, &a));
        any_find(a, "mod_test", "test_f");
        ascheduler_start(&s, a, 0);

        ascheduler_run_once(&s);

        REQUIRE(any_count(a) == 1);
        CHECK_THAT(any_check_string(a, any_check_index(a, 0)),
            Catch::Equals("bad index 0"));
    }

    SECTION("bad_index_underflow")
    {
        aasm_module_push(&as, "test_f");
        aint_t lnew = aasm_add_import(&as, "std-buffer", "new/1");
        aint_t lget = aasm_add_import(&as, "std-buffer", "get/2");
        aint_t lresize = aasm_add_import(&as, "std-buffer", "resize/2");

        aasm_emit(&as, ai_imp(lnew));
        aasm_emit(&as, ai_lsi(8));
        aasm_emit(&as, ai_ivk(1));

        aasm_emit(&as, ai_imp(lresize));
        aasm_emit(&as, ai_lsi(1));
        aasm_emit(&as, ai_llv(0));
        aasm_emit(&as, ai_ivk(2));
        aasm_emit(&as, ai_pop(1));

        aasm_emit(&as, ai_imp(lget));
        aasm_emit(&as, ai_lsi(-1));
        aasm_emit(&as, ai_llv(0));
        aasm_emit(&as, ai_ivk(2));

        aasm_emit(&as, ai_ret());
        aasm_pop(&as);
        aasm_save(&as);

        REQUIRE(AERR_NONE ==
            aloader_add_chunk(&s.loader, as.chunk, as.chunk_size, NULL, NULL));
        REQUIRE(AERR_NONE == aloader_link(&s.loader, TRUE));

        aactor_t* a;
        REQUIRE(AERR_NONE == ascheduler_new_actor(&s, CSTACK_SZ, &a));
        any_find(a, "mod_test", "test_f");
        ascheduler_start(&s, a, 0);

        ascheduler_run_once(&s);

        REQUIRE(any_count(a) == 1);
        CHECK_THAT(any_check_string(a, any_check_index(a, 0)),
            Catch::Equals("bad index -1"));
    }

    SECTION("bad_index_overflow")
    {
        aasm_module_push(&as, "test_f");
        aint_t lnew = aasm_add_import(&as, "std-buffer", "new/1");
        aint_t lget = aasm_add_import(&as, "std-buffer", "get/2");
        aint_t lresize = aasm_add_import(&as, "std-buffer", "resize/2");

        aasm_emit(&as, ai_imp(lnew));
        aasm_emit(&as, ai_lsi(8));
        aasm_emit(&as, ai_ivk(1));

        aasm_emit(&as, ai_imp(lresize));
        aasm_emit(&as, ai_lsi(1));
        aasm_emit(&as, ai_llv(0));
        aasm_emit(&as, ai_ivk(2));
        aasm_emit(&as, ai_pop(1));

        aasm_emit(&as, ai_imp(lget));
        aasm_emit(&as, ai_lsi(1));
        aasm_emit(&as, ai_llv(0));
        aasm_emit(&as, ai_ivk(2));

        aasm_emit(&as, ai_ret());
        aasm_pop(&as);
        aasm_save(&as);

        REQUIRE(AERR_NONE ==
            aloader_add_chunk(&s.loader, as.chunk, as.chunk_size, NULL, NULL));
        REQUIRE(AERR_NONE == aloader_link(&s.loader, TRUE));

        aactor_t* a;
        REQUIRE(AERR_NONE == ascheduler_new_actor(&s, CSTACK_SZ, &a));
        any_find(a, "mod_test", "test_f");
        ascheduler_start(&s, a, 0);

        ascheduler_run_once(&s);

        REQUIRE(any_count(a) == 1);
        CHECK_THAT(any_check_string(a, any_check_index(a, 0)),
            Catch::Equals("bad index 1"));
    }

    SECTION("bad_index_not_a_integer")
    {
        aasm_module_push(&as, "test_f");
        aint_t lnew = aasm_add_import(&as, "std-buffer", "new/1");
        aint_t lget = aasm_add_import(&as, "std-buffer", "get/2");
        aint_t lresize = aasm_add_import(&as, "std-buffer", "resize/2");

        aasm_emit(&as, ai_imp(lnew));
        aasm_emit(&as, ai_lsi(8));
        aasm_emit(&as, ai_ivk(1));

        aasm_emit(&as, ai_imp(lresize));
        aasm_emit(&as, ai_lsi(1));
        aasm_emit(&as, ai_llv(0));
        aasm_emit(&as, ai_ivk(2));
        aasm_emit(&as, ai_pop(1));

        aasm_emit(&as, ai_imp(lget));
        aasm_emit(&as, ai_nil());
        aasm_emit(&as, ai_llv(0));
        aasm_emit(&as, ai_ivk(2));

        aasm_emit(&as, ai_ret());
        aasm_pop(&as);
        aasm_save(&as);

        REQUIRE(AERR_NONE ==
            aloader_add_chunk(&s.loader, as.chunk, as.chunk_size, NULL, NULL));
        REQUIRE(AERR_NONE == aloader_link(&s.loader, TRUE));

        aactor_t* a;
        REQUIRE(AERR_NONE == ascheduler_new_actor(&s, CSTACK_SZ, &a));
        any_find(a, "mod_test", "test_f");
        ascheduler_start(&s, a, 0);

        ascheduler_run_once(&s);

        REQUIRE(any_count(a) == 1);
        CHECK_THAT(any_check_string(a, any_check_index(a, 0)),
            Catch::Equals("not a integer"));
    }

    ascheduler_cleanup(&s);
    aasm_cleanup(&as);
}

TEST_CASE("std_buffer_binding_set")
{
    enum { NUM_IDX_BITS = 4 };
    enum { NUM_GEN_BITS = 4 };

    aasm_t as;
    aasm_init(&as, &myalloc, NULL);
    REQUIRE(aasm_load(&as, NULL) == AERR_NONE);
    add_test_module(&as);

    ascheduler_t s;

    REQUIRE(AERR_NONE ==
        ascheduler_init(&s, NUM_IDX_BITS, NUM_GEN_BITS, &myalloc, NULL));

    astd_lib_add_buffer(&s.loader);

    SECTION("normal")
    {
        aasm_module_push(&as, "test_f");
        aint_t lnew = aasm_add_import(&as, "std-buffer", "new/1");
        aint_t lset = aasm_add_import(&as, "std-buffer", "set/3");
        aint_t lresize = aasm_add_import(&as, "std-buffer", "resize/2");

        aasm_emit(&as, ai_imp(lnew));
        aasm_emit(&as, ai_lsi(8));
        aasm_emit(&as, ai_ivk(1));

        aasm_emit(&as, ai_imp(lresize));
        aasm_emit(&as, ai_lsi(1));
        aasm_emit(&as, ai_llv(0));
        aasm_emit(&as, ai_ivk(2));
        aasm_emit(&as, ai_pop(1));

        aasm_emit(&as, ai_imp(lset));
        aasm_emit(&as, ai_lsi(0));
        aasm_emit(&as, ai_lsi(0));
        aasm_emit(&as, ai_llv(0));
        aasm_emit(&as, ai_ivk(3));

        aasm_emit(&as, ai_ret());
        aasm_pop(&as);
        aasm_save(&as);

        REQUIRE(AERR_NONE ==
            aloader_add_chunk(&s.loader, as.chunk, as.chunk_size, NULL, NULL));
        REQUIRE(AERR_NONE == aloader_link(&s.loader, TRUE));

        aactor_t* a;
        REQUIRE(AERR_NONE == ascheduler_new_actor(&s, CSTACK_SZ, &a));
        any_find(a, "mod_test", "test_f");
        ascheduler_start(&s, a, 0);

        ascheduler_run_once(&s);

        REQUIRE(any_count(a) == 2);
        REQUIRE(any_type(a, any_check_index(a, 1)).type == AVT_NIL);
        REQUIRE(any_type(a, any_check_index(a, 0)).type == AVT_NIL);
    }

    SECTION("bad_no_elements")
    {
        aasm_module_push(&as, "test_f");
        aint_t lnew = aasm_add_import(&as, "std-buffer", "new/1");
        aint_t lset = aasm_add_import(&as, "std-buffer", "set/3");

        aasm_emit(&as, ai_imp(lnew));
        aasm_emit(&as, ai_lsi(8));
        aasm_emit(&as, ai_ivk(1));

        aasm_emit(&as, ai_imp(lset));
        aasm_emit(&as, ai_lsi(0));
        aasm_emit(&as, ai_lsi(0));
        aasm_emit(&as, ai_llv(0));
        aasm_emit(&as, ai_ivk(3));

        aasm_emit(&as, ai_ret());
        aasm_pop(&as);
        aasm_save(&as);

        REQUIRE(AERR_NONE ==
            aloader_add_chunk(&s.loader, as.chunk, as.chunk_size, NULL, NULL));
        REQUIRE(AERR_NONE == aloader_link(&s.loader, TRUE));

        aactor_t* a;
        REQUIRE(AERR_NONE == ascheduler_new_actor(&s, CSTACK_SZ, &a));
        any_find(a, "mod_test", "test_f");
        ascheduler_start(&s, a, 0);

        ascheduler_run_once(&s);

        REQUIRE(any_count(a) == 1);
        CHECK_THAT(any_check_string(a, any_check_index(a, 0)),
            Catch::Equals("bad index 0"));
    }

    SECTION("bad_index_underflow")
    {
        aasm_module_push(&as, "test_f");
        aint_t lnew = aasm_add_import(&as, "std-buffer", "new/1");
        aint_t lset = aasm_add_import(&as, "std-buffer", "set/3");
        aint_t lresize = aasm_add_import(&as, "std-buffer", "resize/2");

        aasm_emit(&as, ai_imp(lnew));
        aasm_emit(&as, ai_lsi(8));
        aasm_emit(&as, ai_ivk(1));

        aasm_emit(&as, ai_imp(lresize));
        aasm_emit(&as, ai_lsi(1));
        aasm_emit(&as, ai_llv(0));
        aasm_emit(&as, ai_ivk(2));
        aasm_emit(&as, ai_pop(1));

        aasm_emit(&as, ai_imp(lset));
        aasm_emit(&as, ai_lsi(0));
        aasm_emit(&as, ai_lsi(-1));
        aasm_emit(&as, ai_llv(0));
        aasm_emit(&as, ai_ivk(3));

        aasm_emit(&as, ai_ret());
        aasm_pop(&as);
        aasm_save(&as);

        REQUIRE(AERR_NONE ==
            aloader_add_chunk(&s.loader, as.chunk, as.chunk_size, NULL, NULL));
        REQUIRE(AERR_NONE == aloader_link(&s.loader, TRUE));

        aactor_t* a;
        REQUIRE(AERR_NONE == ascheduler_new_actor(&s, CSTACK_SZ, &a));
        any_find(a, "mod_test", "test_f");
        ascheduler_start(&s, a, 0);

        ascheduler_run_once(&s);

        REQUIRE(any_count(a) == 1);
        CHECK_THAT(any_check_string(a, any_check_index(a, 0)),
            Catch::Equals("bad index -1"));
    }

    SECTION("bad_index_overflow")
    {
        aasm_module_push(&as, "test_f");
        aint_t lnew = aasm_add_import(&as, "std-buffer", "new/1");
        aint_t lset = aasm_add_import(&as, "std-buffer", "set/3");
        aint_t lresize = aasm_add_import(&as, "std-buffer", "resize/2");

        aasm_emit(&as, ai_imp(lnew));
        aasm_emit(&as, ai_lsi(8));
        aasm_emit(&as, ai_ivk(1));

        aasm_emit(&as, ai_imp(lresize));
        aasm_emit(&as, ai_lsi(1));
        aasm_emit(&as, ai_llv(0));
        aasm_emit(&as, ai_ivk(2));
        aasm_emit(&as, ai_pop(1));

        aasm_emit(&as, ai_imp(lset));
        aasm_emit(&as, ai_lsi(0));
        aasm_emit(&as, ai_lsi(1));
        aasm_emit(&as, ai_llv(0));
        aasm_emit(&as, ai_ivk(3));

        aasm_emit(&as, ai_ret());
        aasm_pop(&as);
        aasm_save(&as);

        REQUIRE(AERR_NONE ==
            aloader_add_chunk(&s.loader, as.chunk, as.chunk_size, NULL, NULL));
        REQUIRE(AERR_NONE == aloader_link(&s.loader, TRUE));

        aactor_t* a;
        REQUIRE(AERR_NONE == ascheduler_new_actor(&s, CSTACK_SZ, &a));
        any_find(a, "mod_test", "test_f");
        ascheduler_start(&s, a, 0);

        ascheduler_run_once(&s);

        REQUIRE(any_count(a) == 1);
        CHECK_THAT(any_check_string(a, any_check_index(a, 0)),
            Catch::Equals("bad index 1"));
    }

    SECTION("bad_index_not_a_integer")
    {
        aasm_module_push(&as, "test_f");
        aint_t lnew = aasm_add_import(&as, "std-buffer", "new/1");
        aint_t lset = aasm_add_import(&as, "std-buffer", "set/3");
        aint_t lresize = aasm_add_import(&as, "std-buffer", "resize/2");

        aasm_emit(&as, ai_imp(lnew));
        aasm_emit(&as, ai_lsi(8));
        aasm_emit(&as, ai_ivk(1));

        aasm_emit(&as, ai_imp(lresize));
        aasm_emit(&as, ai_lsi(1));
        aasm_emit(&as, ai_llv(0));
        aasm_emit(&as, ai_ivk(2));
        aasm_emit(&as, ai_pop(1));

        aasm_emit(&as, ai_imp(lset));
        aasm_emit(&as, ai_lsi(0));
        aasm_emit(&as, ai_nil());
        aasm_emit(&as, ai_llv(0));
        aasm_emit(&as, ai_ivk(3));

        aasm_emit(&as, ai_ret());
        aasm_pop(&as);
        aasm_save(&as);

        REQUIRE(AERR_NONE ==
            aloader_add_chunk(&s.loader, as.chunk, as.chunk_size, NULL, NULL));
        REQUIRE(AERR_NONE == aloader_link(&s.loader, TRUE));

        aactor_t* a;
        REQUIRE(AERR_NONE == ascheduler_new_actor(&s, CSTACK_SZ, &a));
        any_find(a, "mod_test", "test_f");
        ascheduler_start(&s, a, 0);

        ascheduler_run_once(&s);

        REQUIRE(any_count(a) == 1);
        CHECK_THAT(any_check_string(a, any_check_index(a, 0)),
            Catch::Equals("not a integer"));
    }

    SECTION("bad_value_not_a_integer")
    {
        aasm_module_push(&as, "test_f");
        aint_t lnew = aasm_add_import(&as, "std-buffer", "new/1");
        aint_t lset = aasm_add_import(&as, "std-buffer", "set/3");
        aint_t lresize = aasm_add_import(&as, "std-buffer", "resize/2");

        aasm_emit(&as, ai_imp(lnew));
        aasm_emit(&as, ai_lsi(8));
        aasm_emit(&as, ai_ivk(1));

        aasm_emit(&as, ai_imp(lresize));
        aasm_emit(&as, ai_lsi(1));
        aasm_emit(&as, ai_llv(0));
        aasm_emit(&as, ai_ivk(2));
        aasm_emit(&as, ai_pop(1));

        aasm_emit(&as, ai_imp(lset));
        aasm_emit(&as, ai_nil());
        aasm_emit(&as, ai_lsi(0));
        aasm_emit(&as, ai_llv(0));
        aasm_emit(&as, ai_ivk(3));

        aasm_emit(&as, ai_ret());
        aasm_pop(&as);
        aasm_save(&as);

        REQUIRE(AERR_NONE ==
            aloader_add_chunk(&s.loader, as.chunk, as.chunk_size, NULL, NULL));
        REQUIRE(AERR_NONE == aloader_link(&s.loader, TRUE));

        aactor_t* a;
        REQUIRE(AERR_NONE == ascheduler_new_actor(&s, CSTACK_SZ, &a));
        any_find(a, "mod_test", "test_f");
        ascheduler_start(&s, a, 0);

        ascheduler_run_once(&s);

        REQUIRE(any_count(a) == 1);
        CHECK_THAT(any_check_string(a, any_check_index(a, 0)),
            Catch::Equals("not a integer"));
    }

    ascheduler_cleanup(&s);
    aasm_cleanup(&as);
}

TEST_CASE("std_buffer_binding_size")
{
    enum { NUM_IDX_BITS = 4 };
    enum { NUM_GEN_BITS = 4 };

    aasm_t as;
    aasm_init(&as, &myalloc, NULL);
    REQUIRE(aasm_load(&as, NULL) == AERR_NONE);
    add_test_module(&as);

    ascheduler_t s;

    REQUIRE(AERR_NONE ==
        ascheduler_init(&s, NUM_IDX_BITS, NUM_GEN_BITS, &myalloc, NULL));

    astd_lib_add_buffer(&s.loader);

    SECTION("not_a_buffer")
    {
        aasm_module_push(&as, "test_f");
        aint_t lsize = aasm_add_import(&as, "std-buffer", "size/1");

        aasm_emit(&as, ai_imp(lsize));
        aasm_emit(&as, ai_nil());
        aasm_emit(&as, ai_ivk(1));

        aasm_emit(&as, ai_ret());
        aasm_pop(&as);
        aasm_save(&as);

        REQUIRE(AERR_NONE ==
            aloader_add_chunk(&s.loader, as.chunk, as.chunk_size, NULL, NULL));
        REQUIRE(AERR_NONE == aloader_link(&s.loader, TRUE));

        aactor_t* a;
        REQUIRE(AERR_NONE == ascheduler_new_actor(&s, CSTACK_SZ, &a));
        any_find(a, "mod_test", "test_f");
        ascheduler_start(&s, a, 0);

        ascheduler_run_once(&s);

        REQUIRE(any_count(a) == 1);
        CHECK_THAT(any_check_string(a, any_check_index(a, 0)),
            Catch::Equals("not a buffer"));
    }

    ascheduler_cleanup(&s);
    aasm_cleanup(&as);
}

TEST_CASE("std_buffer_binding_capacity")
{
    enum { NUM_IDX_BITS = 4 };
    enum { NUM_GEN_BITS = 4 };

    aasm_t as;
    aasm_init(&as, &myalloc, NULL);
    REQUIRE(aasm_load(&as, NULL) == AERR_NONE);
    add_test_module(&as);

    ascheduler_t s;

    REQUIRE(AERR_NONE ==
        ascheduler_init(&s, NUM_IDX_BITS, NUM_GEN_BITS, &myalloc, NULL));

    astd_lib_add_buffer(&s.loader);

    SECTION("not_a_buffer")
    {
        aasm_module_push(&as, "test_f");
        aint_t lcapacity = aasm_add_import(&as, "std-buffer", "capacity/1");

        aasm_emit(&as, ai_imp(lcapacity));
        aasm_emit(&as, ai_nil());
        aasm_emit(&as, ai_ivk(1));

        aasm_emit(&as, ai_ret());
        aasm_pop(&as);
        aasm_save(&as);

        REQUIRE(AERR_NONE ==
            aloader_add_chunk(&s.loader, as.chunk, as.chunk_size, NULL, NULL));
        REQUIRE(AERR_NONE == aloader_link(&s.loader, TRUE));

        aactor_t* a;
        REQUIRE(AERR_NONE == ascheduler_new_actor(&s, CSTACK_SZ, &a));
        any_find(a, "mod_test", "test_f");
        ascheduler_start(&s, a, 0);

        ascheduler_run_once(&s);

        REQUIRE(any_count(a) == 1);
        CHECK_THAT(any_check_string(a, any_check_index(a, 0)),
            Catch::Equals("not a buffer"));
    }

    ascheduler_cleanup(&s);
    aasm_cleanup(&as);
}