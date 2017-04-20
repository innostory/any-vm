/* Copyright (c) 2017 Nguyen Viet Giang. All rights reserved. */
#include <catch.hpp>

#include <stdlib.h>
#include <any/asm.h>
#include <any/errno.h>
#include <any/string_table.h>

#define REQUIRE_STR_EQUALS(a, b) REQUIRE(strcmp(a, b) == 0)

static void* realloc(void*, void* old, int32_t sz)
{
    return ::realloc(old, sz);
}

static void num_vs_capacity_check(aasm_prototype_t* p)
{
    REQUIRE(p->num_instructions <= p->max_instructions);
    REQUIRE(p->num_constants <= p->max_constants);
    REQUIRE(p->num_imports <= p->max_imports);
    REQUIRE(p->num_nesteds <= p->max_nesteds);
}

typedef struct {
    ainstruction_t pop;
    ainstruction_t get_const;
    ainstruction_t get_local;
    ainstruction_t set_local;
    ainstruction_t get_import;
    ainstruction_t get_upvalue;
    ainstruction_t set_upvalue;
    ainstruction_t jump;
    ainstruction_t jin;
    ainstruction_t call;
    ainstruction_t closure;
    ainstruction_t cap_local;
    ainstruction_t cap_upval;
    ainstruction_t close;

    aconstant_t cinteger;
    aconstant_t cstring;
    aconstant_t cfloat;

    aimport_t imp0;
    aimport_t imp1;
    aimport_t imp2;
} basic_test_ctx;

static void basic_add(aasm_t* a, basic_test_ctx& t)
{
    t.pop = ai_pop(rand());
    t.get_const = ai_get_const(rand());
    t.get_local = ai_get_local(rand());
    t.set_local = ai_set_local(rand());
    t.get_import = ai_get_import(rand());
    t.get_upvalue = ai_get_upvalue(rand());
    t.set_upvalue = ai_set_upvalue(rand());
    t.jump = ai_jump(rand());
    t.jin = ai_jump_if_not(rand());
    t.call = ai_call(rand());
    t.closure = ai_closure(rand());
    t.cap_local = ai_capture_local(rand());
    t.cap_upval = ai_capture_upvalue(rand());
    t.close = ai_close(rand());

    REQUIRE(0 == any_asm_emit(a, ai_nop()));
    REQUIRE(1 == any_asm_emit(a, t.pop));
    REQUIRE(2 == any_asm_emit(a, t.get_const));
    REQUIRE(3 == any_asm_emit(a, ai_get_nil()));
    REQUIRE(4 == any_asm_emit(a, ai_get_bool(TRUE)));
    REQUIRE(5 == any_asm_emit(a, ai_get_bool(FALSE)));
    REQUIRE(6 == any_asm_emit(a, t.get_local));
    REQUIRE(7 == any_asm_emit(a, t.set_local));
    REQUIRE(8 == any_asm_emit(a, t.get_import));
    REQUIRE(9 == any_asm_emit(a, t.get_upvalue));
    REQUIRE(10 == any_asm_emit(a, t.set_upvalue));
    REQUIRE(11 == any_asm_emit(a, t.jump));
    REQUIRE(12 == any_asm_emit(a, t.jin));
    REQUIRE(13 == any_asm_emit(a, t.call));
    REQUIRE(14 == any_asm_emit(a, ai_return()));
    REQUIRE(15 == any_asm_emit(a, t.closure));
    REQUIRE(16 == any_asm_emit(a, t.cap_local));
    REQUIRE(17 == any_asm_emit(a, t.cap_upval));
    REQUIRE(18 == any_asm_emit(a, t.close));

    t.cinteger = ac_integer(rand());
    t.cstring = ac_string(any_asm_string_to_ref(a, "test_const"));
    t.cfloat = ac_float((afloat_t)rand());

    REQUIRE(0 == any_asm_add_constant(a, t.cinteger));
    REQUIRE(1 == any_asm_add_constant(a, t.cstring));
    REQUIRE(2 == any_asm_add_constant(a, t.cfloat));

    t.imp0 = aimport(
        any_asm_string_to_ref(a, "test_imp0_module"), 
        any_asm_string_to_ref(a, "test_imp0_name"));
    t.imp1 = aimport(
        any_asm_string_to_ref(a, "test_imp1_module"),
        any_asm_string_to_ref(a, "test_imp1_name"));
    t.imp2 = aimport(
        any_asm_string_to_ref(a, "test_imp2_module"),
        any_asm_string_to_ref(a, "test_imp2_name"));

    REQUIRE(0 == any_asm_add_import(a, t.imp0));
    REQUIRE(1 == any_asm_add_import(a, t.imp1));
    REQUIRE(2 == any_asm_add_import(a, t.imp2));
};

static void basic_check(aasm_t* a, basic_test_ctx& t)
{
    auto p = any_asm_prototype(a);
    auto c = any_asm_resolve(a);

    num_vs_capacity_check(p);

    REQUIRE(p->num_instructions == 19);
    REQUIRE(c.instructions[0].base.opcode == AOC_NOP);
    REQUIRE(c.instructions[1].base.opcode == AOC_POP);
    REQUIRE(c.instructions[1].pop.n == t.pop.pop.n);
    REQUIRE(c.instructions[2].base.opcode == AOC_GET_CONST);
    REQUIRE(c.instructions[2].get_const.idx == t.get_const.get_const.idx);
    REQUIRE(c.instructions[3].base.opcode == AOC_GET_NIL);
    REQUIRE(c.instructions[4].base.opcode == AOC_GET_BOOL);
    REQUIRE(c.instructions[4].get_bool.val == TRUE);
    REQUIRE(c.instructions[5].base.opcode == AOC_GET_BOOL);
    REQUIRE(c.instructions[5].get_bool.val == FALSE);
    REQUIRE(c.instructions[6].base.opcode == AOC_GET_LOCAL);
    REQUIRE(c.instructions[6].get_local.idx == t.get_local.get_local.idx);
    REQUIRE(c.instructions[7].base.opcode == AOC_SET_LOCAL);
    REQUIRE(c.instructions[7].set_local.idx == t.set_local.set_local.idx);
    REQUIRE(c.instructions[8].base.opcode == AOC_GET_IMPORT);
    REQUIRE(c.instructions[8].get_import.idx == t.get_import.get_import.idx);
    REQUIRE(c.instructions[9].base.opcode == AOC_GET_UPVALUE);
    REQUIRE(c.instructions[9].get_upvalue.idx == t.get_upvalue.get_upvalue.idx);
    REQUIRE(c.instructions[10].base.opcode == AOC_SET_UPVALUE);
    REQUIRE(c.instructions[10].set_upvalue.idx == t.set_upvalue.set_upvalue.idx);
    REQUIRE(c.instructions[11].base.opcode == AOC_JUMP);
    REQUIRE(c.instructions[11].jump.displacement == t.jump.jump.displacement);
    REQUIRE(c.instructions[12].base.opcode == AOC_JUMP_IF_NOT);
    REQUIRE(c.instructions[12].jump_if_not.displacement ==
        t.jin.jump_if_not.displacement);
    REQUIRE(c.instructions[13].base.opcode == AOC_CALL);
    REQUIRE(c.instructions[13].call.nargs == t.call.call.nargs);
    REQUIRE(c.instructions[14].base.opcode == AOC_RETURN);
    REQUIRE(c.instructions[15].base.opcode == AOC_CLOSURE);
    REQUIRE(c.instructions[15].closure.idx == t.closure.closure.idx);
    REQUIRE(c.instructions[16].base.opcode == AOC_CAPTURE_LOCAL);
    REQUIRE(c.instructions[16].capture_local.idx ==
        t.cap_local.capture_local.idx);
    REQUIRE(c.instructions[17].base.opcode == AOC_CAPTURE_UPVALUE);
    REQUIRE(c.instructions[17].capture_upvalue.idx ==
        t.cap_upval.capture_upvalue.idx);
    REQUIRE(c.instructions[18].base.opcode == AOC_CLOSE);
    REQUIRE(c.instructions[18].close.offset == t.close.close.offset);

    REQUIRE(p->num_constants == 3);
    REQUIRE(c.constants[0].b.type == ACT_INTEGER);
    REQUIRE(c.constants[0].i.val == t.cinteger.i.val);
    REQUIRE(c.constants[1].b.type == ACT_STRING);
    REQUIRE(c.constants[1].s.ref == t.cstring.s.ref);
    REQUIRE(c.constants[2].b.type == ACT_FLOAT);
    REQUIRE(c.constants[2].f.val == t.cfloat.f.val);

    REQUIRE(p->num_imports == 3);
    REQUIRE(c.imports[0].module == t.imp0.module);
    REQUIRE(c.imports[0].name == t.imp0.name);
    REQUIRE(c.imports[1].module == t.imp1.module);
    REQUIRE(c.imports[1].name == t.imp1.name);
    REQUIRE(c.imports[2].module == t.imp2.module);
    REQUIRE(c.imports[2].name == t.imp2.name);
}

static void require_equals(aasm_t* a1, aasm_t* a2)
{
    auto p1 = any_asm_prototype(a1);
    auto p2 = any_asm_prototype(a2);
    auto c1 = any_asm_resolve(a1);
    auto c2 = any_asm_resolve(a2);

    REQUIRE_STR_EQUALS(
        any_st_to_string(a1->st, p1->source_name),
        any_st_to_string(a2->st, p2->source_name));
    REQUIRE_STR_EQUALS(
        any_st_to_string(a1->st, p1->module_name),
        any_st_to_string(a2->st, p2->module_name));
    REQUIRE_STR_EQUALS(
        any_st_to_string(a1->st, p1->exported),
        any_st_to_string(a2->st, p2->exported));

    REQUIRE(p1->num_upvalues == p2->num_upvalues);
    REQUIRE(p1->num_arguments == p2->num_arguments);
    REQUIRE(p1->num_local_vars == p2->num_local_vars);
    REQUIRE(p1->num_nesteds == p2->num_nesteds);

    REQUIRE(p1->num_instructions == p2->num_instructions);
    REQUIRE(
        memcmp(
            c1.instructions, 
            c2.instructions, 
            p1->num_instructions*sizeof(ainstruction_t)) == 0);

    REQUIRE(p1->num_constants == p2->num_constants);
    for (int32_t i = 0; i < p1->num_constants; ++i) {
        REQUIRE(c1.constants[i].b.type == c2.constants[i].b.type);
        switch (c1.constants[i].b.type) {
        case ACT_INTEGER:
            REQUIRE(c1.constants[i].i.val == c2.constants[i].i.val);
            break;
        case ACT_STRING:
            REQUIRE_STR_EQUALS(
                any_st_to_string(a1->st, c1.constants[i].s.ref),
                any_st_to_string(a2->st, c2.constants[i].s.ref));
            break;
        case ACT_FLOAT:
            REQUIRE(c1.constants[i].f.val == c2.constants[i].f.val);
            break;
        }
    }

    REQUIRE(p1->num_imports == p2->num_imports);
    for (int32_t i = 0; i < p2->num_imports; ++i) {
        REQUIRE_STR_EQUALS(
            any_st_to_string(a1->st, c1.imports[i].module),
            any_st_to_string(a2->st, c2.imports[i].module));
        REQUIRE_STR_EQUALS(
            any_st_to_string(a1->st, c1.imports[i].name),
            any_st_to_string(a2->st, c2.imports[i].name));
    }
}

TEST_CASE("asm_empty")
{
    aasm_t a;
    any_asm_init(&a, &realloc, NULL);
    REQUIRE(any_asm_load(&a, NULL) == AERR_NONE);
    auto p = any_asm_prototype(&a);

    REQUIRE(p->source_name == 0);
    REQUIRE(p->module_name == 0);
    REQUIRE(p->exported == 0);
    REQUIRE(p->num_instructions == 0);
    REQUIRE(p->num_upvalues == 0);
    REQUIRE(p->num_arguments == 0);
    REQUIRE(p->num_local_vars == 0);
    REQUIRE(p->num_constants == 0);
    REQUIRE(p->num_imports == 0);
    REQUIRE(p->num_nesteds == 0);

    num_vs_capacity_check(p);

    any_asm_cleanup(&a);
}

TEST_CASE("asm_basic")
{
    aasm_t a;
    any_asm_init(&a, &realloc, NULL);
    REQUIRE(any_asm_load(&a, NULL) == AERR_NONE);

    basic_test_ctx t;
    basic_add(&a, t);
    basic_check(&a, t);

    any_asm_cleanup(&a);
}

TEST_CASE("asm_nested")
{
    aasm_t a;
    any_asm_init(&a, &realloc, NULL);
    REQUIRE(any_asm_load(&a, NULL) == AERR_NONE);

    enum { PUSH_COUNT = 25 };

    for (int32_t i = 0; i < ANY_ASM_MAX_NESTED_LEVEL; ++i) {
        for (int32_t j = 0; j < PUSH_COUNT; ++j) {
            REQUIRE(j == any_asm_push(&a));
            basic_test_ctx t;
            basic_add(&a, t);
            basic_check(&a, t);
            REQUIRE(j == any_asm_pop(&a));
            any_asm_open(&a, j);
            basic_check(&a, t);
            REQUIRE(j == any_asm_pop(&a));
        }
        REQUIRE(PUSH_COUNT == any_asm_push(&a));
    }

    for (int32_t i = ANY_ASM_MAX_NESTED_LEVEL - 1; i >= 0; --i) {
        REQUIRE(PUSH_COUNT == any_asm_pop(&a));
    }

    any_asm_cleanup(&a);
}

TEST_CASE("asm_save_load")
{
    aasm_t a1;
    any_asm_init(&a1, &realloc, NULL);
    REQUIRE(any_asm_load(&a1, NULL) == AERR_NONE);

    enum { PUSH_COUNT = 5 };

    for (int32_t i = 0; i < ANY_ASM_MAX_NESTED_LEVEL; ++i) {
        for (int32_t j = 0; j < PUSH_COUNT; ++j) {
            REQUIRE(j == any_asm_push(&a1));
            basic_test_ctx t;
            basic_add(&a1, t);
            basic_check(&a1, t);
            REQUIRE(j == any_asm_pop(&a1));
        }
        REQUIRE(PUSH_COUNT == any_asm_push(&a1));
    }

    any_asm_save(&a1);

    for (int32_t i = ANY_ASM_MAX_NESTED_LEVEL - 1; i >= 0; --i) {
        REQUIRE(PUSH_COUNT == any_asm_pop(&a1));
    }

    aasm_t a2;
    any_asm_init(&a2, &realloc, NULL);
    REQUIRE(any_asm_load(&a2, a1.chunk) == AERR_NONE);

    for (int32_t i = 0; i < ANY_ASM_MAX_NESTED_LEVEL; ++i) {
        for (int32_t j = 0; j < PUSH_COUNT; ++j) {
            any_asm_open(&a1, j);
            any_asm_open(&a2, j);
            require_equals(&a1, &a2);
            REQUIRE(j == any_asm_pop(&a1));
            REQUIRE(j == any_asm_pop(&a2));
        }
        any_asm_open(&a1, PUSH_COUNT);
        any_asm_open(&a2, PUSH_COUNT);
    }

    any_asm_cleanup(&a1);
    any_asm_cleanup(&a2);
}