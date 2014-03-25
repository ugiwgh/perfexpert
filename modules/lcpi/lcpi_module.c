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

#ifdef __cplusplus
extern "C" {
#endif

/* System standard headers */

/* Module headers */
#include "modules/perfexpert_module_base.h"
#include "modules/hpctoolkit/hpctoolkit_module.h"
#include "lcpi_module.h"
#include "lcpi.h"

/* PerfExpert common headers */
#include "common/perfexpert_constants.h"
#include "common/perfexpert_list.h"
#include "common/perfexpert_output.h"

/* Global variable to define the module itself */
perfexpert_module_lcpi_t myself_module;
my_module_globals_t my_module_globals;
char module_version[] = "1.0.0";

/* module_load */
int module_load(void) {
    OUTPUT_VERBOSE((5, "%s", _MAGENTA("loaded")));
    return PERFEXPERT_SUCCESS;
}

/* module_init */
int module_init(void) {
    /* Module pre-requisites */
    if (PERFEXPERT_SUCCESS != perfexpert_module_requires("lcpi",
        PERFEXPERT_PHASE_MEASURE, "hpctoolkit", PERFEXPERT_PHASE_MEASURE,
        PERFEXPERT_MODULE_AFTER)) {
        OUTPUT(("%s", _ERROR("pre-required module/phase not available")));
        return PERFEXPERT_ERROR;
    }
    if (PERFEXPERT_SUCCESS != perfexpert_module_requires("lcpi",
        PERFEXPERT_PHASE_ANALYZE, "hpctoolkit",
        PERFEXPERT_PHASE_MEASURE, PERFEXPERT_MODULE_BEFORE)) {
        OUTPUT(("%s", _ERROR("pre-required module not available")));
        return PERFEXPERT_ERROR;
    }

    /* Initialize list of events */
    perfexpert_list_construct(&(my_module_globals.profiles));
    my_module_globals.metrics_by_name = NULL;
    my_module_globals.threshold = 0.0;
    my_module_globals.help_only = PERFEXPERT_FALSE;
    my_module_globals.mic = PERFEXPERT_FALSE;

    /* Parse module options */
    if (PERFEXPERT_SUCCESS != parse_module_args(myself_module.argc,
        myself_module.argv)) {
        OUTPUT(("%s", _ERROR("parsing module arguments")));
        return PERFEXPERT_ERROR;
    }

    OUTPUT_VERBOSE((5, "%s", _MAGENTA("initialized")));

    return PERFEXPERT_SUCCESS;
}

/* module_fini */
int module_fini(void) {
    OUTPUT_VERBOSE((5, "%s", _MAGENTA("finalized")));
    return PERFEXPERT_SUCCESS;
}

/* module_measure */
int module_measure(void) {
    OUTPUT(("%s", _YELLOW("Setting performance events")));

    my_module_globals.hpctoolkit = (perfexpert_module_hpctoolkit_t *)
        perfexpert_module_get("hpctoolkit");

    /* Should we run this program on the MIC or on the host? */
    if (PERFEXPERT_FALSE == my_module_globals.mic) {
        if (PERFEXPERT_SUCCESS != metrics_generate()) {
            OUTPUT(("%s", _ERROR("unable to generate LCPI metrics")));
            return PERFEXPERT_ERROR;
        }
    } else {
        if (PERFEXPERT_SUCCESS != metrics_generate_mic()) {
            OUTPUT(("%s", _ERROR("unable to generate MIC LCPI metrics")));
            return PERFEXPERT_ERROR;
        }
    }

    return PERFEXPERT_SUCCESS;
}

/* module_analyze */
int module_analyze(void) {
    lcpi_profile_t *p = NULL;

    OUTPUT(("%s", _YELLOW("Analyzing measurements")));

    // Wrap all these functions in a SQL transaction

    if (PERFEXPERT_SUCCESS != database_import(&(my_module_globals.profiles),
        my_module_globals.hpctoolkit->name)) {
        OUTPUT(("%s", _ERROR("unable to import HPCToolkit profiles")));
        return PERFEXPERT_ERROR;
    }

    perfexpert_list_for(p, &(my_module_globals.profiles), lcpi_profile_t) {
        if (PERFEXPERT_SUCCESS != logic_lcpi_compute(p)) {
            OUTPUT(("%s", _ERROR("unable to compute LCPI")));
            return PERFEXPERT_ERROR;
        }
    }

    if (NULL != my_module_globals.order) {
        if (PERFEXPERT_SUCCESS != hotspot_sort(&(my_module_globals.profiles))) {
            OUTPUT(("%s", _ERROR("sorting hotspots")));
            return PERFEXPERT_ERROR;
        }
    }

    if (PERFEXPERT_SUCCESS != output_analysis(&(my_module_globals.profiles))) {
        OUTPUT(("%s", _ERROR("printing analysis report")));
        return PERFEXPERT_ERROR;
    }

    if (PERFEXPERT_SUCCESS != database_export(&(my_module_globals.profiles))) {
        OUTPUT(("%s", _ERROR("writing metrics to database")));
        return PERFEXPERT_ERROR;
    }

    return PERFEXPERT_SUCCESS;
}

#ifdef __cplusplus
}
#endif

// EOF
