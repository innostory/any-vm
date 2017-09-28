/* Copyright (c) 2017 Nguyen Viet Giang. All rights reserved. */
#include <any/std.h>

#include <any/actor.h>
#include <any/loader.h>
#include <any/std_string.h>

static void
limport(
    aactor_t* a)
{
    aint_t a_module = any_check_index(a, -1);
    aint_t a_name = any_check_index(a, -2);
    const char* module = any_check_string(a, a_module);
    const char* name = any_check_string(a, a_name);
    any_import(a, module, name);
}

static void
lmsleep(
    aactor_t* a)
{
    aint_t a_msecs = any_check_index(a, -1);
    any_sleep(a, any_check_integer(a, a_msecs) * 1000000);
    any_push_nil(a);
}

static void
lusleep(
    aactor_t* a)
{
    aint_t a_usecs = any_check_index(a, -1);
    any_sleep(a, any_check_integer(a, a_usecs) * 1000);
    any_push_nil(a);
}

static void
lnsleep(
    aactor_t* a)
{
    aint_t a_nsecs = any_check_index(a, -1);
    any_sleep(a, any_check_integer(a, a_nsecs));
    any_push_nil(a);
}

static void
lspawn(
    aactor_t* a)
{
    apid_t pid;
    aint_t a_entry = any_check_index(a, -1);
    any_push_index(a, a_entry);
    any_spawn(a, 1024*32, 0, &pid);
    any_push_pid(a, pid);
}

static alib_func_t funcs[] = {
    { "import/2", &limport },
    { "msleep/1", &lmsleep },
    { "usleep/1", &lusleep },
    { "nsleep/1", &lnsleep },
    { "spawn/1",  &lspawn },
    { NULL, NULL }
};

static alib_t mod = { "std", funcs };

void
astd_lib_add(
    aloader_t* l)
{
    aloader_add_lib(l, &mod);
}
