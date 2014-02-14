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

#include <sched.h>

#include <csignal>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <ctime>

#include <algorithm>
#include <set>
#include <map>

#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>

#include "mrt.h"
#include "cpuid.h"
#include "macpo_record.h"

typedef std::pair<long, short> val_idx_pair;
typedef std::pair<int, short> line_threadid_pair;

typedef short thread_id;
typedef int line_number;

typedef std::map<thread_id, bool> bool_map;
typedef std::map<thread_id, short> short_map;
typedef std::map<thread_id, long*> long_histogram;

typedef std::map<long, bool_map> bool_map_coll;
typedef std::map<long, short_map> short_map_coll;
typedef std::map<long, long_histogram> long_histogram_coll;

static std::set<int> analyzed_loops;

static bool_map_coll overlap_bin;
static short_map_coll branch_bin, align_bin, sstore_align_bin;
static long_histogram_coll tripcount_map;

static short proc = PROC_UNKNOWN;

static std::map<int, int> branch_loop_line_pair;
static std::map<int, std::set<int> > loop_branch_line_pair;
volatile static short lock_var;

static inline void lock() {
    while (__sync_bool_compare_and_swap(&lock_var, 0, 1) == false)
        ;

    asm volatile("lfence" ::: "memory");
}

static inline void unlock() {
    lock_var = 0;
    asm volatile("sfence" ::: "memory");
}

static int getCoreID()
{
	if (coreID != -1)
		return coreID;

	int info[4];
	if (!isCPUIDSupported())
	{
		coreID = 0;
		return coreID;	// default
	}

	if (proc == PROC_AMD)
	{
		__cpuid(info, 1, 0);
		coreID = (info[1] & 0xff000000) >> 24;
		return coreID;
	}
	else if (proc == PROC_INTEL)
	{
		int apic_id = 0;
		__cpuid(info, 0xB, 0);
		if (info[EBX] != 0)	// x2APIC
		{
			__cpuid(info, 0xB, 2);
			apic_id = info[EDX];

			#ifdef DEBUG_PRINT
				fprintf (stderr, "MACPO :: Request from core with x2APIC ID %d\n", apic_id);
			#endif
		}
		else			// Traditonal APIC
		{
			__cpuid(info, 1, 0);
			apic_id = (info[EBX] & 0xff000000) >> 24;

			#ifdef DEBUG_PRINT
				fprintf (stderr, "MACPO :: Request from core with legacy APIC ID %d\n", apic_id);
			#endif
		}

		int i;
		for (i=0; i<numCores; i++)
			if (apic_id == intel_apic_mapping[i])
				break;

		coreID = i == numCores ? 0 : i;
		return coreID;
	}

	coreID = 0;
	return coreID;
}

static void signalHandler(int sig)
{
	// Reset the signal handler
	signal(sig, signalHandler);

	struct itimerval itimer_old, itimer_new;
	itimer_new.it_interval.tv_sec = 0;
	itimer_new.it_interval.tv_usec = 0;

	if (sleeping == 1)
	{
		// Wake up for a brief period of time
		fdatasync(fd);
		write(fd, &terminal_node, sizeof(node_t));

		// Don't reorder so that `sleeping = 0' remains after fwrite()
		asm volatile("" ::: "memory");
		sleeping = 0;

		itimer_new.it_value.tv_sec = AWAKE_SEC;
		itimer_new.it_value.tv_usec = AWAKE_USEC;
		setitimer(ITIMER_PROF, &itimer_new, &itimer_old);
	}
	else
	{
		// Go to sleep now...
		sleeping = 1;

		int temp = sleep_sec + new_sleep_sec;
		sleep_sec = new_sleep_sec;
		new_sleep_sec = temp;

		itimer_new.it_value.tv_sec = 3+sleep_sec*3;
		itimer_new.it_value.tv_usec = 0;
		setitimer(ITIMER_PROF, &itimer_new, &itimer_old);
		access_count = 0;
	}
}

static void set_thread_affinity() {
    // Get which processor is this running on
    int info[4];
    if (!isCPUIDSupported())
        fprintf (stderr, "MACPO :: CPUID not supported, cannot determine processor core information, resorting to defaults...\n");
    else
    {
        char processorName[13];
        getProcessorName(processorName);

        if (strncmp(processorName, "AuthenticAMD", 12) == 0)		proc = PROC_AMD;
        else if (strncmp(processorName, "GenuineIntel", 12) == 0)	proc = PROC_INTEL;
        else								proc = PROC_UNKNOWN;

        if (proc == PROC_UNKNOWN)
            fprintf (stderr, "MACPO :: Cannot determine processor identification, resorting to defaults...\n");
        else if (proc == PROC_INTEL)	// We need to do some special set up for Intel machines
        {
            numCores = sysconf(_SC_NPROCESSORS_CONF);
            intel_apic_mapping = (int*) malloc (sizeof (int) * numCores);

            if (intel_apic_mapping)
            {
                // Get the original affinity mask
                cpu_set_t old_mask;
                CPU_ZERO(&old_mask);
                sched_getaffinity(0, sizeof(cpu_set_t), &old_mask);

                // Loop over all cores and find map their APIC IDs to core IDs
                for (int i=0; i<numCores; i++)
                {
                    cpu_set_t mask;
                    CPU_ZERO(&mask);
                    CPU_SET(i, &mask);

                    if (sched_setaffinity(0, sizeof(cpu_set_t), &mask) != -1)
                    {
                        // Get the APIC ID
                        __cpuid(info, 0xB, 0);
                        if (info[EBX] != 0)	// x2APIC
                        {
                            __cpuid(info, 0xB, 2);
                            intel_apic_mapping[i] = info[EDX];
                        }
                        else			// Traditonal APIC
                        {
                            __cpuid(info, 1, 0);
                            intel_apic_mapping[i] = (info[EBX] & 0xff000000) >> 24;
                        }

#ifdef DEBUG_PRINT
                        fprintf (stderr, "MACPO :: Registered mapping from core %d to APIC %d\n", i, intel_apic_mapping[i]);
#endif
                    }
                }

                // Reset the original affinity mask
                sched_setaffinity (0, sizeof(cpu_set_t), &old_mask);
            }
#ifdef DEBUG_PRINT
            else
            {
                fprintf (stderr, "MACPO :: malloc() failed\n");
            }
#endif
        }
    }
}

static void create_output_file() {
    char szFilename[32];
    sprintf (szFilename, "macpo.%d.out", (int) getpid());
    fd = open(szFilename, O_CREAT | O_APPEND | O_WRONLY | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP);
    if (fd < 0)
    {
        perror("MACPO :: Error opening log for writing");
        exit(1);
    }

    if (access("macpo.out", F_OK) == 0)
    {
        // file exists, remove it
        if (unlink("macpo.out") == -1)
            perror ("MACPO :: Failed to remove macpo.out");
    }

    // Now create the symlink
    if (symlink(szFilename, "macpo.out") == -1)
        perror ("MACPO :: Failed to create symlink \"macpo.out\"");

    // Now that we are done handling the critical stuff, write the metadata log to the macpo.out file
    node_t node;
    node.type_message = MSG_METADATA;
    size_t exe_path_len = readlink ("/proc/self/exe", node.metadata_info.binary_name, STRING_LENGTH-1);
    if (exe_path_len == -1)
        perror ("MACPO :: Failed to read name of the binary from /proc/self/exe");
    else
    {
        // Write the terminating character
        node.metadata_info.binary_name[exe_path_len]='\0';
        time(&node.metadata_info.execution_timestamp);

        write(fd, &node, sizeof(node_t));
    }

    terminal_node.type_message = MSG_TERMINAL;
}

static void set_timers() {
	// Set up the signal handler
	signal(SIGPROF, signalHandler);

	struct itimerval itimer_old, itimer_new;
	itimer_new.it_interval.tv_sec = 0;
	itimer_new.it_interval.tv_usec = 0;

	if (sleep_sec != 0)
	{
		sleeping = 1;

		itimer_new.it_value.tv_sec = 3+sleep_sec*4;
		itimer_new.it_value.tv_usec = 0;
		setitimer(ITIMER_PROF, &itimer_new, &itimer_old);
	}
	else
	{
		sleeping = 0;

		itimer_new.it_value.tv_sec = AWAKE_SEC;
		itimer_new.it_value.tv_usec = AWAKE_USEC;
		setitimer(ITIMER_PROF, &itimer_new, &itimer_old);
	}
}

void indigo__init_(short create_file, short enable_sampling)
{
    set_thread_affinity();

    if (create_file) {
        create_output_file();

        if (enable_sampling) {
            // We could set the timers even if the file did not have to be
            // create.  But timers are used to write to the file only. Since
            // there is no other use of timers, we disable setting up timers as
            // well.
            set_timers();
        } else {
            // Explicitly set awake mode to ON.
            sleeping = 0;
        }
    }

	atexit(indigo__exit);
}

static bool index_comparator(const val_idx_pair& v1, const val_idx_pair& v2) {
    return v1.first < v2.first;
}

static void print_tripcount_histogram(int line_number, long* histogram) {
    val_idx_pair pair_histogram[MAX_HISTOGRAM_ENTRIES];
    for (int i=0; i<MAX_HISTOGRAM_ENTRIES; i++) {
        pair_histogram[i] = val_idx_pair(histogram[i], i);
    }

    std::sort(pair_histogram, pair_histogram + MAX_HISTOGRAM_ENTRIES);
    if (pair_histogram[MAX_HISTOGRAM_ENTRIES-1].first > 0) {
        // At least one non-zero entry.
        int values[3] = { pair_histogram[MAX_HISTOGRAM_ENTRIES-1].second,
            pair_histogram[MAX_HISTOGRAM_ENTRIES-2].second,
            pair_histogram[MAX_HISTOGRAM_ENTRIES-3].second
        };

        long counts[3] = { pair_histogram[MAX_HISTOGRAM_ENTRIES-1].first,
            pair_histogram[MAX_HISTOGRAM_ENTRIES-2].first,
            pair_histogram[MAX_HISTOGRAM_ENTRIES-3].first
        };

        fprintf (stderr, "  count %d%s found %ld time(s), ", values[0],
                values[0] == MAX_HISTOGRAM_ENTRIES-1 ? "+" : "", counts[0]);
        if (counts[1]) {
            fprintf (stderr, "count %d%s found %ld time(s), ", values[1],
                    values[1] == MAX_HISTOGRAM_ENTRIES-1 ? "+" : "", counts[1]);

            if (counts[2]) {
                fprintf (stderr, "count %d%s found %ld time(s), ", values[2],
                        values[2] == MAX_HISTOGRAM_ENTRIES-1 ? "+" : "",
                        counts[2]);
            }
        }

        fprintf (stderr, "\n");
    }
}

static bool nonzero_alignment(int line_number, long* histogram) {
    for (int i=1; i<CACHE_LINE_SIZE; i++) {
        if (histogram[i] > 0)
            return true;
    }

    return false;
}

static void print_alignment_histogram(int line_number, long* histogram) {
    val_idx_pair pair_histogram[CACHE_LINE_SIZE];
    for (int i=0; i<CACHE_LINE_SIZE; i++) {
        pair_histogram[i] = val_idx_pair(histogram[i], i);
    }

    std::sort(pair_histogram, pair_histogram + CACHE_LINE_SIZE);
    if (pair_histogram[CACHE_LINE_SIZE-1].first > 0) {
        // At least one non-zero entry.
        int values[3] = { pair_histogram[CACHE_LINE_SIZE-1].second,
            pair_histogram[CACHE_LINE_SIZE-2].second,
            pair_histogram[CACHE_LINE_SIZE-3].second
        };

        long counts[3] = { pair_histogram[CACHE_LINE_SIZE-1].first,
            pair_histogram[CACHE_LINE_SIZE-2].first,
            pair_histogram[CACHE_LINE_SIZE-3].first
        };

        fprintf (stderr, "  count %d%s found %ld time(s), ", values[0],
                values[0] == MAX_HISTOGRAM_ENTRIES-1 ? "+" : "", counts[0]);
        if (counts[1]) {
            fprintf (stderr, "count %d%s found %ld time(s), ", values[1],
                    values[1] == MAX_HISTOGRAM_ENTRIES-1 ? "+" : "", counts[1]);

            if (counts[2]) {
                fprintf (stderr, "count %d%s found %ld time(s), ", values[2],
                        values[2] == MAX_HISTOGRAM_ENTRIES-1 ? "+" : "",
                        counts[2]);
            }
        }

        fprintf (stderr, "\n");
    }
}

static void indigo__exit()
{
	if (fd >= 0)	close(fd);
	if (intel_apic_mapping)	free(intel_apic_mapping);

    for (std::set<int>::iterator it = analyzed_loops.begin();
            it != analyzed_loops.end(); it++) {
        const int& line_number = *it;

        fprintf (stderr, "\n==== Loop at line %d ====\n", line_number);

        if (sstore_align_bin.find(line_number) != sstore_align_bin.end()) {
            short_map& map = sstore_align_bin[line_number];
            for (short_map::iterator it = map.begin(); it != map.end();
                    it++) {
                short align_status = it->second;
                switch(align_status) {
                    case FULL_ALIGNED:
                        fprintf (stderr, "all non-temporal arrays align, use "
                            "__assume_aligned() or #pragma vector aligned to "
                            "tell compiler about alignment.\n");
                        break;

                    case MUTUAL_ALIGNED:
                        fprintf (stderr, "all non-temporal arrays are mutually "
                            "aligned but not aligned to cache-line boundary, "
                            "use _mm_malloc/_mm_free to allocate/free aligned "
                            "storage.\n");
                        break;

                    case NOT_ALIGNED:
                        fprintf (stderr, "non-temporal arrays are not aligned, "
                            "try using _mm_malloc/_mm_free to allocate/free "
                            "arrays that are aligned with cache-line bounary.");
                        break;
                }
            }
        }

        if (align_bin.find(line_number) != align_bin.end()) {
            short_map& map = align_bin[line_number];
            for (short_map::iterator it = map.begin(); it != map.end();
                    it++) {
                short align_status = it->second;
                switch(align_status) {
                    case FULL_ALIGNED:
                        fprintf (stderr, "all arrays align, use "
                            "__assume_aligned() or #pragma vector aligned to "
                            "tell compiler about alignment.\n");
                        break;

                    case MUTUAL_ALIGNED:
                        fprintf (stderr, "all arrays are mutually aligned but "
                            "not aligned to cache-line boundary, use "
                            "_mm_malloc/_mm_free to allocate/free aligned "
                            "storage.\n");
                        break;

                    case NOT_ALIGNED:
                        fprintf (stderr, "arrays are not aligned, try using "
                            "_mm_malloc/_mm_free to allocate/free arrays that "
                            "are aligned with cache-line bounary.");
                        break;
                }
            }
        }

        if (overlap_bin.find(line_number) != overlap_bin.end()) {
            bool_map& map = overlap_bin[line_number];
            for (bool_map::iterator it = map.begin(); it != map.end(); it++) {
                bool overlap = it->second;

                if (overlap) {
                    fprintf (stderr, "Use `restrict' keyword.\n");
                }
            }
        }

        if (tripcount_map.find(line_number) != tripcount_map.end()) {
            fprintf (stderr, "trip count: ");
            long_histogram& histogram = tripcount_map[line_number];
            for (long_histogram::iterator it = histogram.begin();
                    it != histogram.end(); it++) {
                long* histogram = it->second;

                if (histogram) {
                    print_tripcount_histogram(line_number, histogram);
                    free(histogram);
                }
            }
        }

        if (loop_branch_line_pair.find(line_number) !=
                loop_branch_line_pair.end()) {
            std::set<int>& branch_lines = loop_branch_line_pair[line_number];
            for (std::set<int>::iterator it = branch_lines.begin();
                    it != branch_lines.end(); it++) {
                const int& branch_line_number = *it;

                if (branch_bin.find(branch_line_number) != branch_bin.end()) {
                    short_map& map = branch_bin[branch_line_number];
                    for (short_map::iterator it = map.begin(); it != map.end(); it++) {
                        short branch_status = it->second;

                        switch(branch_status) {
                            case BRANCH_UNKNOWN:
                                fprintf (stderr, "branch at line %d is "
                                        "unpredictable.\n", branch_line_number);
                                break;

                            case BRANCH_MOSTLY_FALSE:
                                fprintf (stderr, "branch at line %d mostly "
                                        "evaluates to false.\n",
                                        branch_line_number);
                                break;

                            case BRANCH_MOSTLY_TRUE:
                                fprintf (stderr, "branch at line %d mostly "
                                        "evaluates to true.\n",
                                        branch_line_number);
                                break;

                            case BRANCH_FALSE:
                                fprintf (stderr, "branch at line %d always "
                                        "evaluates to false.\n",
                                        branch_line_number);
                                break;

                            case BRANCH_TRUE:
                                fprintf (stderr, "branch at line %d always "
                                        "evaluates to true.\n",
                                        branch_line_number);
                                break;
                        }
                    }
                }
            }
        }
    }
}

static inline void fill_trace_struct(int read_write, int line_number, size_t base, size_t p, int var_idx)
{
	// If this process was never supposed to record stats
	// or if the file-open failed, then return
	if (fd < 0)	return;

	if (sleeping == 1 || access_count >= 131072)	// 131072 is 128*1024 (power of two)
		return;

	size_t address_base = (size_t) base;
	size_t address = (size_t) p;

	node_t node;
	node.type_message = MSG_TRACE_INFO;

	node.trace_info.coreID = getCoreID();
	node.trace_info.read_write = read_write;
	node.trace_info.base = address_base;
	node.trace_info.address = address;
	node.trace_info.var_idx = var_idx;
	node.trace_info.line_number = line_number;

	write(fd, &node, sizeof(node_t));
}

static inline void fill_mem_struct(int read_write, int line_number, size_t p, int var_idx, int type_size)
{
	// If this process was never supposed to record stats
	// or if the file-open failed, then return
	if (fd < 0)	return;

	if (sleeping == 1 || access_count >= 131072)	// 131072 is 128*1024 (power of two)
		return;

	node_t node;
	node.type_message = MSG_MEM_INFO;

	node.mem_info.coreID = getCoreID();
	node.mem_info.read_write = read_write;
	node.mem_info.address = p;
	node.mem_info.var_idx = var_idx;
	node.mem_info.line_number = line_number;
	node.mem_info.type_size = type_size;

	write(fd, &node, sizeof(node_t));
}

void indigo__gen_trace_c(int read_write, int line_number, void* base, void* addr, int var_idx)
{
	if (fd >= 0)	fill_trace_struct(read_write, line_number, (size_t) base, (size_t) addr, var_idx);
}

void indigo__gen_trace_f(int *read_write, int *line_number, void* base, void* addr, int *var_idx)
{
	if (fd >= 0)	fill_trace_struct(*read_write, *line_number, (size_t) base, (size_t) addr, *var_idx);
}

short& get_branch_bin(int line_number, int loop_line_number) {
    analyzed_loops.insert(loop_line_number);
    short core_id = getCoreID();

    // Obtain lock.
    lock();

    long* return_value = NULL;
    // Check if we need to allocate a new histogram.
    short_map_coll::iterator it = branch_bin.find(line_number);
    if (it == branch_bin.end()) {
        short_map& map = branch_bin[line_number];
        short_map::iterator it = map.find(core_id);
        if (it == map.end()) {
            map[core_id] = BRANCH_NOINIT;
        }
    } else {
        short_map& map = it->second;
        short_map::iterator it = map.find(core_id);
        if (it == map.end()) {
            map[core_id] = BRANCH_NOINIT;
        }
    }

    // Release lock.
    unlock();

    return branch_bin[line_number][core_id];
}

void indigo__record_branch_c(int line_number, int loop_line_number, int true_branch_count, int false_branch_count)
{
    branch_loop_line_pair[line_number] = loop_line_number;
    loop_branch_line_pair[loop_line_number].insert(line_number);

    int status = BRANCH_UNKNOWN;
    int branch_count = true_branch_count + false_branch_count;
    if (true_branch_count != 0 && false_branch_count == 0) {
        status = BRANCH_TRUE;
    } else if (true_branch_count * 100.0f > branch_count * 85.0f) {
        status = BRANCH_MOSTLY_TRUE;
    } else if (false_branch_count != 0 > true_branch_count == 0) {
        status = BRANCH_FALSE;
    } else if (false_branch_count * 100.0f > branch_count * 85.0f) {
        status = BRANCH_MOSTLY_FALSE;
    } else {
        status = BRANCH_UNKNOWN;
    }

    if (get_branch_bin(line_number, loop_line_number) != BRANCH_UNKNOWN) {
        get_branch_bin(line_number, loop_line_number) = status;
    }
}

#if 0
void indigo__simd_branch_c(int line_number, int idxv, int type_size, int branch_dir, int common_alignment, int* recorded_simd_branch_dir)
{
	if (common_alignment >= 0 && type_size > 0) {
        int simd_index = (idxv*type_size - common_alignment) % 64;
        if (simd_index >= 0) {
            if (get_branch_bin(line_number) != BRANCH_UNKNOWN) {
                if (*recorded_simd_branch_dir == -1)
                    *recorded_simd_branch_dir = branch_dir;

                if (simd_index > 0) {
                    // Check if branch_dir is the same as previously recorded dirs.
                    if (*recorded_simd_branch_dir != branch_dir) {
                        get_branch_bin(line_number) = BRANCH_UNKNOWN;
                    }
                } else {
                    // Set the branch_dir value for subsequent iterations.
                    if (*recorded_simd_branch_dir != branch_dir)
                        get_branch_bin(line_number) = BRANCH_SIMD;
                    *recorded_simd_branch_dir = branch_dir;
                }
            }
        }
    }
}
#endif

void indigo__vector_stride_c(int loop_line_number, int var_idx, void* addr, int type_size) {
	// If this process was never supposed to record stats
	// or if the file-open failed, then return
	if (fd < 0)	return;

	if (sleeping == 1 || access_count >= 131072)	// 131072 is 128*1024 (power of two)
		return;

	node_t node;
	node.type_message = MSG_VECTOR_STRIDE_INFO;

	node.vector_stride_info.coreID = getCoreID();
	node.vector_stride_info.address = (size_t) addr;
	node.vector_stride_info.var_idx = var_idx;
	node.vector_stride_info.loop_line_number = loop_line_number;
	node.vector_stride_info.type_size = type_size;

	write(fd, &node, sizeof(node_t));
}

void indigo__record_c(int read_write, int line_number, void* addr, int var_idx, int type_size)
{
	if (fd >= 0)	fill_mem_struct(read_write, line_number, (size_t) addr, var_idx, type_size);
}

void indigo__record_f_(int *read_write, int *line_number, void* addr, int *var_idx, int* type_size)
{
	if (fd >= 0)	fill_mem_struct(*read_write, *line_number, (size_t) addr, *var_idx, *type_size);
}

void indigo__write_idx_c(const char* var_name, const int length)
{
	node_t node;
	node.type_message = MSG_STREAM_INFO;
#define	indigo__MIN(a,b)	(a) < (b) ? (a) : (b)
	int dst_len = indigo__MIN(STREAM_LENGTH-1, length);
#undef indigo__MIN

	strncpy(node.stream_info.stream_name, var_name, dst_len);
	node.stream_info.stream_name[dst_len] = '\0';

	write(fd, &node, sizeof(node_t));
}

void indigo__write_idx_f_(const char* var_name, const int* length)
{
	indigo__write_idx_c(var_name, *length);
}

long* new_histogram(size_t histogram_entries) {
    long* histogram = (long*) malloc(sizeof(long) * histogram_entries);
    if (histogram) {
        memset(histogram, 0, sizeof(long) * histogram_entries);
    }

    return histogram;
}

bool& get_overlap_bin(int line_number) {
    analyzed_loops.insert(line_number);
    short core_id = getCoreID();

    // Obtain lock.
    lock();

    long* return_value = NULL;
    // Check if we need to allocate a new histogram.
    bool_map_coll::iterator it = overlap_bin.find(line_number);
    if (it == overlap_bin.end()) {
        bool_map& map = overlap_bin[line_number];
        bool_map::iterator it = map.find(core_id);
        if (it == map.end()) {
            map[core_id] = 0;
        }
    } else {
        bool_map& map = it->second;
        bool_map::iterator it = map.find(core_id);
        if (it == map.end()) {
            map[core_id] = 0;
        }
    }

    // Release lock.
    unlock();

    return overlap_bin[line_number][core_id];
}

#define MAX_ADDR    128

/**
    Helper function to allocate / get histogramss for alignment checking.
*/

long* get_histogram(long_histogram_coll& collection, int line_number,
        size_t histogram_entries) {
    short core_id = getCoreID();

    // Obtain lock.
    lock();

    long* return_value = NULL;
    // Check if we need to allocate a new histogram.
    long_histogram_coll::iterator it = collection.find(line_number);
    if (it == collection.end()) {
        long_histogram& histogram = collection[line_number];
        long_histogram::iterator it = histogram.find(core_id);
        if (it == histogram.end()) {
            histogram[core_id] = new_histogram(histogram_entries);
            return_value = histogram[core_id];
        } else {
            return_value = it->second;
        }
    } else {
        long_histogram& histogram = it->second;
        long_histogram::iterator it = histogram.find(core_id);
        if (it == histogram.end()) {
            histogram[core_id] = new_histogram(histogram_entries);
            return_value = histogram[core_id];
        } else {
            return_value = it->second;
        }
    }

    // Release lock.
    unlock();

    return return_value;
}

short& get_sstore_alignment_bin(int line_number) {
    analyzed_loops.insert(line_number);
    short core_id = getCoreID();

    // Obtain lock.
    lock();

    long* return_value = NULL;
    // Check if we need to allocate a new histogram.
    short_map_coll::iterator it = sstore_align_bin.find(line_number);
    if (it == sstore_align_bin.end()) {
        short_map& map = sstore_align_bin[line_number];
        short_map::iterator it = map.find(core_id);
        if (it == map.end()) {
            map[core_id] = ALIGN_NOINIT;
        }
    } else {
        short_map& map = it->second;
        short_map::iterator it = map.find(core_id);
        if (it == map.end()) {
            map[core_id] = ALIGN_NOINIT;
        }
    }

    // Release lock.
    unlock();

    return sstore_align_bin[line_number][core_id];
}

short& get_alignment_bin(int line_number) {
    analyzed_loops.insert(line_number);
    short core_id = getCoreID();

    // Obtain lock.
    lock();

    long* return_value = NULL;
    // Check if we need to allocate a new histogram.
    short_map_coll::iterator it = align_bin.find(line_number);
    if (it == align_bin.end()) {
        short_map& map = align_bin[line_number];
        short_map::iterator it = map.find(core_id);
        if (it == map.end()) {
            map[core_id] = ALIGN_NOINIT;
        }
    } else {
        short_map& map = it->second;
        short_map::iterator it = map.find(core_id);
        if (it == map.end()) {
            map[core_id] = ALIGN_NOINIT;
        }
    }

    // Release lock.
    unlock();

    return align_bin[line_number][core_id];
}

long* get_tripcount_histogram(int line_number) {
    analyzed_loops.insert(line_number);
    return get_histogram(tripcount_map, line_number, MAX_HISTOGRAM_ENTRIES);
}

/**
    indigo__aligncheck_c()
    Checks for alignment to cache line boundary and memory overlap.
    Returns common alignment, if any. Otherwise, returns -1.
*/
int indigo__aligncheck_c(int line_number, /* int* type_size, */ int stream_count, ...) {
    va_list args;

    void* start_list[MAX_ADDR] = {0};
    void* end_list[MAX_ADDR] = {0};

    if (stream_count >= MAX_ADDR) {
#if 0
        fprintf (stderr, "MACPO :: Stream count too large, truncating to %d "
                " for memory disambiguation.\n", MAX_ADDR);
#endif

        stream_count = MAX_ADDR-1;
    }

    int common_alignment = -1, last_remainder = 0;

    int i, j;
    va_start(args, stream_count);

    for (i=0; i<stream_count; i++) {
        void* start = va_arg(args, void*);
        void* end = va_arg(args, void*);
        size_t size = va_arg(args, size_t);

        int remainder = ((long) start) % 64;

        if (i == 0) {
            common_alignment = remainder;
            // *type_size = size;
        } else {
            if (common_alignment != remainder)
                common_alignment = -1;

#if 0
            if (*type_size != size)
                *type_size = -1;
#endif
        }

#if 0
        if (((long) start) % 64) {
            fprintf (stderr, "MACPO :: Reference in loop at line %d is not "
                    "aligned at cache line boundary.\n", line_number);
        }
#endif

        // Really simple and inefficient (n^2) algorightm to check duplicates.
        bool overlap = false;
        for (j=0; j<i && overlap == false; j++) {
#if 0
            fprintf (stderr, "i: %p-%p, compared against: %p-%p = %d.\n",
                    start_list[j], end_list[j], start, end, start_list[j] > end || end_list[j] < start);
#endif
            if (!(start_list[j] > end || end_list[j] < start)) {
                overlap = true;
            }
        }

        if (get_overlap_bin(line_number) != true) {
            get_overlap_bin(line_number) = overlap;
        }

#if 0
        if (overlap) {
            fprintf (stderr, "MACPO :: Found overlap among references for loop "
                    "at line %d.\n", line_number);
        }
#endif

        start_list[i] = start;
        end_list[i] = end;
    }

    va_end(args);

    if (common_alignment == -1) {
        get_alignment_bin(line_number) = NOT_ALIGNED;
    } else if (common_alignment == 0) {
        if (get_alignment_bin(line_number) == ALIGN_NOINIT) {
            get_alignment_bin(line_number) = FULL_ALIGNED;
        } else {
            // Leave the existing measurement as it is.
        }
    } else {
        if (get_alignment_bin(line_number) == ALIGN_NOINIT || get_alignment_bin(line_number) == FULL_ALIGNED) {
            get_alignment_bin(line_number) = MUTUAL_ALIGNED;
        } else {
            // Leave the existing measurement as it is.
        }
    }

    return common_alignment;
}

/**
    indigo__sstore_aligncheck_c()
    Checks for alignment of streaming stores to cache line boundary.
*/

void indigo__sstore_aligncheck_c(int line_number, int stream_count, ...) {
    va_list args;

    int common_alignment = -1, last_remainder = 0;

    int i, j;
    va_start(args, stream_count);

    for (i=0; i<stream_count; i++) {
        void* address = va_arg(args, void*);
        int remainder = ((long) address) % 64;

        if (i == 0) {
            common_alignment = remainder;
        } else {
            if (common_alignment != remainder)
                common_alignment = -1;
        }
    }

    va_end(args);

    if (common_alignment == -1) {
        get_sstore_alignment_bin(line_number) = NOT_ALIGNED;
    } else if (common_alignment == 0) {
        if (get_sstore_alignment_bin(line_number) == ALIGN_NOINIT) {
            get_sstore_alignment_bin(line_number) = FULL_ALIGNED;
        } else {
            // Leave the existing measurement as it is.
        }
    } else {
        if (get_sstore_alignment_bin(line_number) == ALIGN_NOINIT || get_alignment_bin(line_number) == FULL_ALIGNED) {
            get_sstore_alignment_bin(line_number) = MUTUAL_ALIGNED;
        } else {
            // Leave the existing measurement as it is.
        }
    }
}

void indigo__tripcount_check_c(int line_number, long trip_count) {
    long* histogram = get_tripcount_histogram(line_number);
    if (histogram == NULL)
        return;

    if (trip_count < 0)
        trip_count = 0;

    if (trip_count >= MAX_HISTOGRAM_ENTRIES)
        trip_count = MAX_HISTOGRAM_ENTRIES-1;

    histogram[trip_count]++;
}
