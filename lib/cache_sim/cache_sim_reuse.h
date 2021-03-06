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

#ifndef CACHE_SIM_REUSE_H_
#define CACHE_SIM_REUSE_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef CACHE_SIM_H_
#include "cache_sim.h"
#endif

#ifndef _STDINT_H
#include <stdint.h>
#endif

/* Functions declaration */
int cache_sim_reuse_enable(cache_handle_t *cache, const uint64_t limit);
int cache_sim_reuse_disable(cache_handle_t *cache);
int cache_sim_reuse_limited(cache_handle_t *cache, const uint64_t lineid);
int cache_sim_reuse_unlimited(cache_handle_t *cache, const uint64_t lineid);
uint64_t cache_sim_reuse_get_age(cache_handle_t *cache, const uint64_t lineid);

#ifdef __cplusplus
}
#endif

#endif /* CACHE_SIM_REUSE_H_ */
