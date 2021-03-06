/*
 * Copyright (c) 2011-2013  University of Texas at Austin. All rights reserved.
 *
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * This file is part of PerfExpert.
 *
 * PerfExpert is free software: you can redistribute it and/or modify it under
 * the terms of the The University of Texas at Austin Research License
 *
 * PerfExpert is distributed in the hope that it will be useful, but WITHOUT ANY
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
 * A PARTICULAR PURPOSE.
 *
 * Authors: Leonardo Fialho and Ashay Rane
 *
 * $HEADER$
 */

#ifndef CACHE_SIM_TYPES_H_
#define CACHE_SIM_TYPES_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef _STDLIB_H
#include <stdlib.h>
#endif

#ifndef _STDINT_H
#include <stdint.h>
#endif

#ifndef CACHE_SIM_H_
#include "cache_sim.h"
#endif

/* Type declaration: list item and base type for items that are put in lists */
typedef struct list_item list_item_t;
struct list_item {
    volatile list_item_t *next;
    volatile list_item_t *prev;
};

/* Type declaration: list and base type for list containers */
typedef struct list {
    list_item_t head;
    volatile uint32_t len;
} list_t;

/* Type declaration: reuse list item (64 bytes) */
typedef struct {
    volatile list_item_t *next;
    volatile list_item_t *prev;
    uint64_t line_id;
    uint16_t age;
    uint16_t padding[3]; // can be safely used for something else
} list_item_reuse_t;

/* Type declaration: symbol list item (128 bytes) */
typedef struct {
    /* list basic type and list of caches this symbol spans to (40 bytes) */
    volatile list_item_t *next;
    volatile list_item_t *prev;
    list_t lines;
    /* performance counters (48 bytes) */
    uint64_t access;
    uint64_t hit;
    uint64_t miss;
    uint64_t conflict;
    uint64_t prefetcher_hit;
    uint64_t prefetcher_evict;
    /* the symbols itself (40 bytes) */
    char symbol[CACHE_SIM_SYMBOL_MAX_LENGTH];
} list_item_symbol_t;

/* Type declaration: enum to hold different types of prefetcher */
typedef enum {
    PREFETCHER_NEXT_LINE_SINGLE, // prefetch the next line when a miss occurs
    PREFETCHER_NEXT_LINE_TAGGED, // prefetch the next line when a miss occurs
                                 // and keep prefechting if a hit happens on a
                                 // prefetched line
    // PREFETCHER_STREAM,
    PREFETCHER_INVALID
} prefetcher_t;

/* Type declaration: the reason why a line have been loaded into the cache */
typedef enum {
    LOAD_ACCESS,
    LOAD_PREFETCH
} load_t;

/* Function type declarations */
typedef int (*reuse_fn_t)(cache_handle_t *, const uint64_t);
typedef int (*policy_init_fn_t)(cache_handle_t *);
typedef int (*policy_access_fn_t)(cache_handle_t *, const uint64_t,
    const load_t);

/* Type declaration: cache structure */
struct cache_handle {
    /* make the cache 'listable' */
    volatile list_item_t *next;
    volatile list_item_t *prev;
    /* provided information */
    int total_size;
    int line_size;
    int associativity;
    /* deducted information */
    int total_lines;
    int total_sets;
    int offset_length;
    int set_length;
    /* replacement policy (or algorithm) */
    policy_access_fn_t access_fn;
    /* data section (replacement algorithm dependent) */
    void *data;
    /* reuse distance data */
    void *reuse_data;
    uint64_t reuse_limit;
    reuse_fn_t reuse_fn;
    /* symbols tracking data */
    void *symbol_data;
    /* prefetchers */
    int next_line;
    /* cache hierarchy */
    int level;
    list_t upper;
    list_t lower;
    /* performance counters */
    uint64_t access;               // # of cache hits (inclusive)
    uint64_t hit;                  // # of cache misses (inclusive)
    uint64_t miss;                 // # of cache accesses (inclusive)
    uint64_t conflict;             // # of set associative conflicts
    uint64_t prefetcher_next_line; // # of lines loaded by this prefetcher
    uint64_t prefetcher_hit;       // # of prefetched lines hit
    uint64_t prefetcher_evict;     // # of evicted lines loaded by prefetcher
};

/* Type declaration: policy structure */
typedef struct {
    const char *name;
    policy_init_fn_t init_fn;
    policy_access_fn_t access_fn;
} policy_t;

#ifdef __cplusplus
}
#endif

#endif /* CACHE_SIM_TYPES_H_ */
