/*
 * Copyright (c) 2011-2015  University of Texas at Austin. All rights reserved.
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
#include <stdio.h>
#include <string.h>

/* Utility headers */
#include <sqlite3.h>

/* Modules headers */
#include "macpo.h"

#include "macpo_module.h"

#include "../perfexpert_module_base.h"

/* PerfExpert common headers */
#include "common/perfexpert_alloc.h"
#include "common/perfexpert_output.h"
#include "common/perfexpert_fork.h"
#include "common/perfexpert_util.h"

//perfexpert_module_t myself_module;

/* macpo_instrument_all */
int macpo_instrument_all(void) {
    char *error = NULL, sql[MAX_BUFFER_SIZE];

    OUTPUT_VERBOSE((2, "%s", _BLUE("Adding MACPO instrumentation")));
    OUTPUT(("%s", _YELLOW("Adding MACPO instrumentation")));

    bzero(sql, MAX_BUFFER_SIZE);
    sprintf(sql, "SELECT name, file, line FROM hotspot WHERE perfexpert_id = "
        "%llu", globals.unique_id);

    if (SQLITE_OK != sqlite3_exec(globals.db, sql, macpo_instrument, NULL,
        &error)) {
        OUTPUT(("%s %s", _ERROR("SQL error"), error));
        sqlite3_free(error);
        return PERFEXPERT_ERROR;
    }

    return PERFEXPERT_SUCCESS;
}

/* macpo_instrument */
static int macpo_instrument(void *n, int c, char **val, char **names) {
    char *t = NULL, *argv[9], *name = val[0], *file = val[1], *line = val[2];
    test_t test;
    int rc;
    char *folder, *fullpath, *filename, *rose_name;

    OUTPUT_VERBOSE((6, "  instrumenting %s   @   %s:%s", name, file, line));

    if (PERFEXPERT_SUCCESS != perfexpert_util_file_is_writable(file)) {
        OUTPUT_VERBOSE((6, "  file %s is not writable", file));
        return PERFEXPERT_SUCCESS;
    }

    // Remove everything after '(' in the function name (if exists)
    char *ptr = strchr(name, '(');
    if (ptr) {
        *ptr = 0;
    }

    //OUTPUT_VERBOSE((6, "short name %s", name));

    if (PERFEXPERT_SUCCESS != perfexpert_util_file_exists(file)) {
        return PERFEXPERT_SUCCESS;
    }

    if (PERFEXPERT_SUCCESS != perfexpert_util_filename_only(file, &filename)) {
        return PERFEXPERT_SUCCESS;
    }

    if (PERFEXPERT_SUCCESS != perfexpert_util_path_only(file, &folder)) {
        return PERFEXPERT_ERROR;
    }

    PERFEXPERT_ALLOC(char, fullpath, (strlen(globals.moduledir) +
                     strlen(folder) + 10));
    snprintf(fullpath, strlen(globals.moduledir) + strlen(folder) + 10,
             "%s/%s", globals.moduledir, folder);

    perfexpert_util_make_path(fullpath);

    /* Remove everyting after the '.' (for OMP functions) */
    /*
    name = t;
    if (strstr(t, ".omp_fn.")) {
        strsep(&t, ".");
    }
    */

    ptr = strstr (name, ".omp_fn.");
    if (ptr) {
        *ptr = 0;
    }

    argv[0] = "macpo.sh";


    if (0 == line) {
        PERFEXPERT_ALLOC(char, argv[1], (strlen(name)+25));
        snprintf(argv[1], strlen(name)+25, "--macpo:check-alignment=%s", name);
        PERFEXPERT_ALLOC(char, argv[2], (strlen(name)+30));
        snprintf(argv[2], strlen(name)+30, "--macpo:record-tripcount=%s", name);
        PERFEXPERT_ALLOC(char, argv[3], (strlen(name)+30));
        snprintf(argv[3], strlen(name)+30, "--macpo:vector-strides=%s", name);
    }
    else{
        PERFEXPERT_ALLOC(char, argv[1], (strlen(name)+30));
        snprintf(argv[1], strlen(name)+30, "--macpo:check-alignment=%s:%s", name, line);
        PERFEXPERT_ALLOC(char, argv[2], (strlen(name)+35));
        snprintf(argv[2], strlen(name)+35, "--macpo:record-tripcount=%s:%s", name, line);
        PERFEXPERT_ALLOC(char, argv[3], (strlen(name)+35));
        snprintf(argv[3], strlen(name)+35, "--macpo:vector-strides=%s:%s", name, line);
    }


    PERFEXPERT_ALLOC(char, argv[4],
        (strlen(globals.moduledir) + strlen(file) + 30));
    snprintf(argv[4], strlen(globals.moduledir) + strlen(file) + 30,
            "--macpo:backup-filename=%s/%s", globals.moduledir, file);

    PERFEXPERT_ALLOC(char, argv[5], (strlen(name) + 25));
    if (0 == line) {
        snprintf(argv[5], strlen(name) + 25, "--macpo:instrument=%s", name);
    } else {
        snprintf(argv[5], strlen(name) + 25, "--macpo:instrument=%s:%s", name,
                 line);
    }

    argv[6] = "--macpo:no-compile";

    PERFEXPERT_ALLOC(char, argv[7], strlen(file)+1);
    snprintf(argv[7], strlen(file)+1,"%s",file);

    argv[8] = NULL;  /* Add NULL to indicate the end of arguments */

    PERFEXPERT_ALLOC(char, test.output, (strlen(globals.moduledir) +
                     strlen(name) + strlen(line) + 20));
    snprintf(test.output, strlen(globals.moduledir) + strlen(name) + strlen(line) + 20,
            "%s/%s-%s-macpo.output", globals.moduledir,
            name, line);
    test.input = NULL;
    test.info = globals.program;

    OUTPUT_VERBOSE((6, "   COMMAND=[%s %s %s %s %s %s %s %s]", argv[0], argv[1],
                    argv[2], argv[3], argv[4], argv[5], argv[6], argv[7]));

    rc = perfexpert_fork_and_wait(&test, (char **)argv);

    /*
    switch (rc) {
        case PERFEXPERT_FAILURE:
        case PERFEXPERT_ERROR:
            OUTPUT(("%s (return code: %d) Usually, this means that an error"
                " happened during the program execution. To see the program"
                "'s output, check the content of this file: [%s]. If you "
                "want to PerfExpert ignore the return code next time you "
                "run this program, set the 'return-code' option for the "
                "macpo module. See 'perfepxert -H macpo' for details.",
                _ERROR("the target program returned non-zero"), rc,
                test.output));
            return PERFEXPERT_ERROR;

        case PERFEXPERT_SUCCESS:
            OUTPUT_VERBOSE((7, "[ %s  ]", _BOLDGREEN("OK")));
            break;

        default:
            break;
    }
    */

    PERFEXPERT_ALLOC(char, rose_name, (strlen(filename) + 6));
    snprintf(rose_name, strlen(filename) + 6, "rose_%s", filename);
    OUTPUT_VERBOSE((9, "Copying file %s to: %s", rose_name, file));


    if (PERFEXPERT_SUCCESS != perfexpert_util_file_rename(rose_name, file)) {
        OUTPUT(("%s impossible to copy file %s to %s", _ERROR("IO ERROR"),
                rose_name, file));
    }

    PERFEXPERT_DEALLOC(test.output);
    PERFEXPERT_DEALLOC(argv[1]);
    PERFEXPERT_DEALLOC(argv[2]);
    PERFEXPERT_DEALLOC(argv[3]);
    PERFEXPERT_DEALLOC(argv[4]);
    PERFEXPERT_DEALLOC(argv[5]);
    PERFEXPERT_DEALLOC(argv[7]);
    PERFEXPERT_DEALLOC(fullpath);
    return PERFEXPERT_SUCCESS;
}

int macpo_compile() {
    int comp_loaded = PERFEXPERT_FALSE;

    if (PERFEXPERT_TRUE == perfexpert_module_available("make")) {
        OUTPUT_VERBOSE((5, "%s",
            _CYAN("will use make as compilation module")));
        myself_module.measurement = (perfexpert_module_measurement_t *) perfexpert_module_get("make");
        if (NULL != myself_module.measurement)
            comp_loaded = PERFEXPERT_TRUE;
    }

    if (comp_loaded == PERFEXPERT_FALSE && PERFEXPERT_TRUE == perfexpert_module_available("icc")) {
        OUTPUT_VERBOSE((5, "%s",
            _CYAN("will use icc as compilation module")));
        myself_module.measurement = (perfexpert_module_measurement_t *) perfexpert_module_get("icc");
        if (NULL != myself_module.measurement)
            comp_loaded = PERFEXPERT_TRUE;
    }

    if (comp_loaded == PERFEXPERT_FALSE && PERFEXPERT_TRUE == perfexpert_module_available("gcc")) {
        OUTPUT_VERBOSE((5, "%s",
            _CYAN("will use gcc as compilation module")));
        myself_module.measurement = (perfexpert_module_measurement_t *) perfexpert_module_get("gcc");
        if (NULL != myself_module.measurement)
            comp_loaded = PERFEXPERT_TRUE;
    }
    if (comp_loaded == PERFEXPERT_FALSE) {
        OUTPUT(("%s", _ERROR("required compilation module not available")));
        return PERFEXPERT_ERROR;
    }

    if (myself_module.measurement->compile() != PERFEXPERT_SUCCESS) {
        OUTPUT(("%s", _ERROR (" failed to compile ")));
        return PERFEXPERT_ERROR;
    }

    return PERFEXPERT_SUCCESS;
}

int macpo_run() {
    char *argv[2];
    int rc = PERFEXPERT_SUCCESS;
    test_t test;

    argv[0] = globals.program_full;
    argv[1] = NULL;

    PERFEXPERT_ALLOC(char, test.output,
        (strlen(globals.moduledir) + 8));
    sprintf(test.output, "%s/macpo-run.output", globals.moduledir);
    test.input  = NULL;
    test.info   = globals.program;

    /* fork_and_wait_and_pray */
    rc = perfexpert_fork_and_wait(&test, argv);
    PERFEXPERT_DEALLOC(test.output);
    switch (rc) {
        case PERFEXPERT_FAILURE:
        case PERFEXPERT_ERROR:
            OUTPUT(("%s (return code: %d) Usually, this means that an error"
                " happened during the program execution. To see the program"
                "'s output, check the content of this file: [%s]. If you "
                "want to PerfExpert ignore the return code next time you "
                "run this program, set the 'return-code' option for the "
                "macpo module. See 'perfepxert -H macpo' for details.",
                _ERROR("the target program returned non-zero"), rc, 
                test.output));
            return PERFEXPERT_ERROR;

        case PERFEXPERT_SUCCESS:
            OUTPUT_VERBOSE((7, "[ %s  ]", _BOLDGREEN("OK")));
            break;

        default:
            break;
    }

    return PERFEXPERT_SUCCESS;
}

int macpo_analyze() {
    char * argv[3];
    int rc;
    test_t test;

    argv[0] = "macpo-analyze";
    argv[1] = "macpo.out";
    argv[2] = NULL;

    PERFEXPERT_ALLOC(char, test.output, (strlen(globals.moduledir) + 2));
    snprintf(test.output, strlen(globals.moduledir) + 2,
            "%s/macpo-analyze.output", globals.moduledir);
    test.input = NULL;
    test.info = globals.program;

    OUTPUT_VERBOSE((6, "   COMMAND=[%s %s]", argv[0], argv[1]));

    rc = perfexpert_fork_and_wait(&test, (char **)argv);

    switch (rc) {
        case PERFEXPERT_FAILURE:
        case PERFEXPERT_ERROR:
            OUTPUT(("%s (return code: %d) Usually, this means that an error"
                " happened during the program execution. To see the program"
                "'s output, check the content of this file: [%s]. If you "
                "want to PerfExpert ignore the return code next time you "
                "run this program, set the 'return-code' option for the "
                "macpo module. See 'perfepxert -H macpo' for details.",
                _ERROR("the target program returned non-zero"), rc,
                test.output));
            return PERFEXPERT_ERROR;

        case PERFEXPERT_SUCCESS:
            OUTPUT_VERBOSE((7, "[ %s  ]", _BOLDGREEN("OK")));
            break;

        default:
            break;
    }

    PERFEXPERT_DEALLOC(test.output);
    return PERFEXPERT_SUCCESS;
}

#ifdef __cplusplus
}
#endif

// EOF
