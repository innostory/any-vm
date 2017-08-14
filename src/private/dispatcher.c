/* Copyright (c) 2017 Nguyen Viet Giang. All rights reserved. */
#include <any/actor.h>

#include <any/gc_string.h>

void actor_dispatch(aactor_t* a)
{
    aframe_t* frame = a->frame;
    aprototype_t* pt = frame->pt;
    aprototype_header_t* pth = pt->header;
    for (; frame->ip < pth->num_instructions; ++frame->ip) {
        ainstruction_t* i = pt->instructions + frame->ip;
        switch (i->b.opcode) {
        case AOC_NOP:
            break;
        case AOC_POP:
            any_pop(a, i->pop.n);
            break;
        case AOC_LDK: {
                aconstant_t* c = pt->constants + i->ldk.idx;
                if (i->ldk.idx < 0 || i->ldk.idx >= pth->num_constants) {
                    any_error(a, AERR_RUNTIME,
                        "bad constant index %d", i->ldk.idx);
                }
                switch (pt->constants[i->ldk.idx].type) {
                case ACT_INTEGER:
                    any_push_integer(a, c->integer);
                    break;
                case ACT_STRING:
                    any_push_string(a, pt->strings + c->string);
                    break;
                case ACT_REAL:
                    any_push_real(a, c->real);
                    break;
                default:
                    any_error(a, AERR_RUNTIME, "bad constant type");
                    break;
                }
            }
            break;
        case AOC_NIL:
            any_push_nil(a);
            break;
        case AOC_LDB:
            any_push_bool(a, i->ldb.val ? TRUE : FALSE);
            break;
        case AOC_LSI:
            any_push_integer(a, i->lsi.val);
            break;
        case AOC_LLV:
            any_push_idx(a, i->llv.idx);
            break;
        case AOC_SLV:
            any_insert(a, i->slv.idx);
            break;
        case AOC_IMP:
            if (i->imp.idx < 0 || i->imp.idx >= pth->num_imports) {
                any_error(a, AERR_RUNTIME, "bad import index %d", i->imp.idx);
            } else {
                aactor_push(a, pt->import_values + i->imp.idx);
            }
            break;
        case AOC_CLS:
            if (i->cls.idx < 0 || i->cls.idx >= pth->num_nesteds) {
                any_error(a, AERR_RUNTIME, "bad nested index %d", i->cls.idx);
            } else {
                avalue_t v;
                v.tag.b = ABT_FUNCTION;
                v.tag.variant = AVTF_AVM;
                v.v.avm_func = pt->nesteds + i->cls.idx;
                aactor_push(a, &v);
            }
            break;
        case AOC_JMP: {
                int32_t nip = frame->ip + i->jmp.displacement + 1;
                if (nip < 0 || nip >= pth->num_instructions) {
                    any_error(a, AERR_RUNTIME, "bad jump");
                } else {
                    frame->ip = nip - 1;
                }
            }
            break;
        case AOC_JIN: {
                int32_t nip;
                any_pop(a, 1);
                if (a->stack[a->sp].tag.b != ABT_BOOL) {
                    any_error(a, AERR_RUNTIME, "condition must be boolean");
                }
                if (a->stack[a->sp].v.boolean) continue;
                nip = frame->ip + i->jin.displacement + 1;
                if (nip < 0 || nip >= pth->num_instructions) {
                    any_error(a, AERR_RUNTIME, "bad jump");
                } else {
                    frame->ip = nip - 1;
                }
            }
            break;
        case AOC_IVK:
            any_call(a, i->ivk.nargs);
            break;
        case AOC_RET:
            return;
        case AOC_SND:
        case AOC_RCV:
        case AOC_RMV:
            any_error(a, AERR_RUNTIME, "TODO");
            break;
        }
    }
    any_error(a, AERR_RUNTIME, "return missing");
}