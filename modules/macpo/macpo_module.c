/*
 * Copyright (c) 2011-2016  University of Texas at Austin. All rights reserved.
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
 * Authors: Antonio Gomez-Iglesias, Leonardo Fialho and Ashay Rane
 *
 * $HEADER$
 */

#ifdef __cplusplus
extern "C" {
#endif

/* System standard headers */

/* Module headers */
#include "macpo_module.h"
#include "macpo.h"

/* PerfExpert common headers */
#include "common/perfexpert_alloc.h"
#include "common/perfexpert_constants.h"
#include "common/perfexpert_output.h"
#include "modules/perfexpert_module_base.h"

/* Global variable to define the module itself */
char module_version[] = "1.0.0";
my_module_globals_t my_module_globals;

/* module_load */
int module_load(void) {
    OUTPUT_VERBOSE((5, "%s", _MAGENTA("loaded")));
    return PERFEXPERT_SUCCESS;
}

/* module_init */
int module_init(void) {
    int comp_loaded = PERFEXPERT_FALSE;
    my_module_globals.prefix[0] = NULL;
    my_module_globals.before[0] = NULL;
    my_module_globals.after[0] = NULL;
    my_module_globals.ignore_return_code = PERFEXPERT_TRUE;
    my_module_globals.threshold = globals.threshold; 
    my_module_globals.instrument.maxfiles = 0;

    /* Module pre-requisites */
    if (PERFEXPERT_SUCCESS != perfexpert_module_requires("macpo",
        PERFEXPERT_PHASE_INSTRUMENT, "lcpi", PERFEXPERT_PHASE_ANALYZE,
        PERFEXPERT_MODULE_BEFORE)) {
        OUTPUT(("%s", _ERROR("pre-required module/phase not available")));
        return PERFEXPERT_ERROR;
    }
    if (PERFEXPERT_SUCCESS != perfexpert_module_requires("macpo",
        PERFEXPERT_PHASE_INSTRUMENT, NULL, PERFEXPERT_PHASE_COMPILE,
        PERFEXPERT_MODULE_CLONE_AFTER)) {
        OUTPUT(("%s", _ERROR("pre-required module/phase not available")));
        return PERFEXPERT_ERROR;
    }
    if (PERFEXPERT_SUCCESS != perfexpert_module_requires("macpo",
        PERFEXPERT_PHASE_MEASURE, "macpo", PERFEXPERT_PHASE_INSTRUMENT,
        PERFEXPERT_MODULE_BEFORE)) {
        OUTPUT(("%s", _ERROR("pre-required module/phase not available")));
        return PERFEXPERT_ERROR;
    }
    if (PERFEXPERT_SUCCESS != perfexpert_module_requires("macpo",
        PERFEXPERT_PHASE_ANALYZE, "macpo", PERFEXPERT_PHASE_MEASURE,
        PERFEXPERT_MODULE_BEFORE)) {
        OUTPUT(("%s", _ERROR("pre-required module/phase not available")));
        return PERFEXPERT_ERROR;
    }

    if (PERFEXPERT_SUCCESS != parse_module_args(myself_module.argc,
        myself_module.argv)) {
        OUTPUT(("%s", _ERROR("parsing module arguments")));
        return PERFEXPERT_ERROR;
    }

    // TODO: I'm going to need this here
    /*
    if (PERFEXPERT_SUCCESS != init_database()) {
        OUTPUT(("%s", _ERROR("initialing tables")));
        return PERFEXPERT_ERROR;
    }
    */

    OUTPUT_VERBOSE((5, "%s", _MAGENTA("initialized")));

    return PERFEXPERT_SUCCESS;
}

/* module_fini */
int module_fini(void) {
    // Deallocate this list
    int i = 0, j = 0;

    for (i = 0; i < my_module_globals.instrument.maxfiles; ++i) {
        for (j = 0; j < my_module_globals.instrument.list[i].maxlocations; ++j) {
            PERFEXPERT_DEALLOC(my_module_globals.instrument.list[i].names[j]);
        }
        PERFEXPERT_DEALLOC(my_module_globals.instrument.list[i].file);
        PERFEXPERT_DEALLOC(my_module_globals.instrument.list[i].backfile);
        my_module_globals.instrument.list[i].maxlocations = 0;
    }   
    my_module_globals.instrument.maxfiles = 0;

    OUTPUT_VERBOSE((5, "%s", _MAGENTA("finalized")));
    return PERFEXPERT_SUCCESS;
}

/* module_instrument */
int module_instrument(void) {

    if (PERFEXPERT_SUCCESS != macpo_instrument_all()) {
        OUTPUT(("%s", _ERROR("instrumenting files")));
        macpo_restore_code();
        macpo_compile();
        return PERFEXPERT_ERROR;
    }

    return PERFEXPERT_SUCCESS;
}

/* module_measure */
int module_measure(void) {
    //First, recompile the code
    if (PERFEXPERT_SUCCESS != macpo_compile()) {
        macpo_restore_code();
        macpo_compile();
        OUTPUT(("%s", _ERROR("compiling code after instrumentation")));
        return PERFEXPERT_ERROR;
    }

    if (PERFEXPERT_SUCCESS != macpo_run()) {
        macpo_restore_code();
        OUTPUT(("%s", _ERROR("running code after instrumentation")));
        return PERFEXPERT_ERROR;
    }
    //Rerun the code
    
    return PERFEXPERT_SUCCESS;
}

/* module_analyze */
int module_analyze(void) {
    if (PERFEXPERT_SUCCESS != macpo_analyze()) {
        macpo_restore_code();
        macpo_compile();
        OUTPUT(("%s", _ERROR("analyzing MACPO result")));
        return PERFEXPERT_ERROR;
    }
    return PERFEXPERT_SUCCESS;
}

#ifdef __cplusplus
}
#endif

// EOF
