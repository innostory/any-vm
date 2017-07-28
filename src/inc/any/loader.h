/* Copyright (c) 2017 Nguyen Viet Giang. All rights reserved. */
#pragma once

#include <any/rt_types.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Initialize as a new byte code loader.
ANY_API void aloader_init(aloader_t* self, aalloc_t alloc, void* alloc_ud);

/** This will not free the `self` pointer itself.
\brief Release all internal allocated memory, result as a *fresh* loader.
*/
ANY_API void aloader_cleanup(aloader_t* self);

/** Add new byte code chunk to `pendings` list.
\note `chunk_alloc` is optional, used to free `chunk` if necessary.
*/
ANY_API int32_t aloader_add_chunk(
    aloader_t* self, achunk_header_t* chunk, int32_t chunk_sz,
    aalloc_t chunk_alloc, void* chunk_alloc_ud);

/// Add new native lib module.
ANY_API void aloader_add_lib(aloader_t* self, alib_t* lib);

/** Link chunks of byte code together.
\brief There is `safe` option that rolling back the state in case of failed.
\return `AERR_NONE` if successful, otherwise cleanup `pendings` chunks.
*/
ANY_API int32_t aloader_link(aloader_t* self, int32_t safe);

/// Free chunks in `garbages` list that are not marked for `retain`.
ANY_API void aloader_sweep(aloader_t* self);

/** Lookup for a module level symbol.
\return `AERR_NONE` if successful.
*/
ANY_API int32_t aloader_find(
    aloader_t* self, const char* module, const char* name, avalue_t* value);

#ifdef __cplusplus
} // extern "C"
#endif