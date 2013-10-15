--
-- Copyright (c) 2011-2013  University of Texas at Austin. All rights reserved.
--
-- $COPYRIGHT$
--
-- Additional copyrights may follow
--
-- This file is part of PerfExpert.
--
-- PerfExpert is free software: you can redistribute it and/or modify it under
-- the terms of the The University of Texas at Austin Research License
-- 
-- PerfExpert is distributed in the hope that it will be useful, but WITHOUT ANY
-- WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR
-- A PARTICULAR PURPOSE.
-- 
-- Authors: Leonardo Fialho and Ashay Rane
--
-- $HEADER$
--

--
-- Drop tables
--
DROP TABLE IF EXISTS "category";
DROP TABLE IF EXISTS "recommendation";
DROP TABLE IF EXISTS "pattern";
DROP TABLE IF EXISTS "transformation";
DROP TABLE IF EXISTS "transformation_pattern";
DROP TABLE IF EXISTS "language";
DROP TABLE IF EXISTS "architecture";
DROP TABLE IF EXISTS "function";
DROP TABLE IF EXISTS "recommendation_architecture";
DROP TABLE IF EXISTS "recommendation_transformation";
DROP TABLE IF EXISTS "recommendation_category";
DROP TABLE IF EXISTS "recommendation_function";
DROP TABLE IF EXISTS "metric";
DROP TABLE IF EXISTS "log_pr";

--
-- Enable foreign keys
--
PRAGMA foreign_keys = ON;

--
-- Create tables
--
CREATE TABLE "category" (
    "id" INTEGER NOT NULL,
    "desc" TEXT(1024) NOT NULL,
    "short" TEXT(16) NOT NULL,
    PRIMARY KEY ("id")
);

--
-- 'loop' field means:
--     0 = [should not be used], 1 = [1 loops (AutoSCOPE's 'loop1')],
--     2 = [2 loops (AutoSCOPE's 'loop2')], [3 = 3 loop (AutoSCOPE's 'loop3')],
--     null = [attribute not set]
-- AutoSCOPE's multiple_functions, multiple_loops and boost were ignored
--
CREATE TABLE "recommendation" (
    "id" INTEGER,
    "desc" CHAR(1024) NOT NULL,
    "reason" CHAR(1024) NOT NULL,
    "example" CHAR(1024),
    "loop" INTEGER,
    PRIMARY KEY ("id")
);

CREATE TABLE "pattern" (
    "id" INTEGER NOT NULL,
    "language" INTEGER NOT NULL,
    "recognizer" TEXT(256) NOT NULL,
    "desc" TEXT(1024) NOT NULL,
    PRIMARY KEY ("id") ,
    CONSTRAINT "fk_pattern_language"
        FOREIGN KEY ("language")
        REFERENCES "language" ("id")
);

CREATE TABLE "transformation" (
    "id" INTEGER NOT NULL,
    "language" INTEGER NOT NULL,
    "transformer" TEXT(256) NOT NULL,
    "desc" TEXT(1024) NOT NULL,
    PRIMARY KEY ("id") ,
    CONSTRAINT "fk_transformation_language"
        FOREIGN KEY ("language")
        REFERENCES "language" ("id")
);

CREATE TABLE "language" (
    "id" INTEGER NOT NULL,
    "desc" TEXT(256) NOT NULL,
    "short" TEXT(16) NOT NULL,
    PRIMARY KEY ("id")
);

CREATE TABLE "architecture" (
    "id" INTEGER NOT NULL,
    "desc" TEXT(1024),
    "short" TEXT(16),
    PRIMARY KEY ("id")
);

CREATE TABLE "function" (
    "id" INTEGER NOT NULL,
    "desc" TEXT(1024) NOT NULL,
    "statement" TEXT(1048576) NOT NULL,
    PRIMARY KEY ("id")
);

CREATE TABLE "recommendation_architecture" (
    "id_recommendation" INTEGER NOT NULL,
    "id_architecture" INTEGER NOT NULL,
    PRIMARY KEY(id_recommendation, id_architecture),
    CONSTRAINT "fk_recommendation_architecture_recommendation"
        FOREIGN KEY ("id_recommendation")
        REFERENCES "recommendation" ("id"),
    CONSTRAINT "fk_recommendation_architecture_architecture" 
        FOREIGN KEY ("id_architecture") 
        REFERENCES "architecture" ("id")
);

CREATE TABLE "recommendation_transformation" (
    "id_recommendation" INTEGER NOT NULL,
    "id_transformation" INTEGER NOT NULL,
    PRIMARY KEY(id_recommendation, id_transformation),
    CONSTRAINT "fk_recommendation_transformation_recommendation" 
        FOREIGN KEY ("id_recommendation") 
        REFERENCES "recommendation" ("id"),
    CONSTRAINT "fk_recommendation_transformation_transformation" 
        FOREIGN KEY ("id_transformation") 
        REFERENCES "transformation" ("id")
);

CREATE TABLE "transformation_pattern" (
    "id_transformation" INTEGER NOT NULL,
    "id_pattern" INTEGER NOT NULL,
    PRIMARY KEY(id_transformation, id_pattern),
    CONSTRAINT "fk_transformation_pattern_transformation" 
        FOREIGN KEY ("id_transformation") 
        REFERENCES "transformation" ("id"),
    CONSTRAINT "fk_transformation_pattern_pattern" 
        FOREIGN KEY ("id_pattern") 
        REFERENCES "pattern" ("id")
);

CREATE TABLE "recommendation_category" (
    "id_recommendation" INTEGER NOT NULL,
    "id_category" INTEGER NOT NULL,
    "weight" REAL DEFAULT 1,
    PRIMARY KEY(id_recommendation, id_category),
    CONSTRAINT "fk_recommendation_category_category" 
        FOREIGN KEY ("id_category")
        REFERENCES "category" ("id"),
    CONSTRAINT "fk_recommendation_category_recommendation" 
        FOREIGN KEY ("id_recommendation") 
        REFERENCES "recommendation" ("id")
);

CREATE TABLE "recommendation_function" (
    "id_recommendation" INTEGER NOT NULL,
    "id_function" INTEGER NOT NULL,
    "weight" REAL,
    PRIMARY KEY(id_recommendation, id_function),
    CONSTRAINT "fk_recommendation_function_recommendation" 
        FOREIGN KEY ("id_recommendation") 
        REFERENCES "recommendation" ("id"),
    CONSTRAINT "fk_recommendation_function_function" 
        FOREIGN KEY ("id_function") 
        REFERENCES "function" ("id")
);

CREATE TABLE "log_pr" (
    "timestamp" DATE DEFAULT (datetime('now','localtime')),
    "pid" INTEGER NOT NULL,
    "code_filename" CHAR(1024) NOT NULL,
    "code_line_number" INTEGER NOT NULL,
    "code_fragment" TEXT(1048576) NOT NULL,
    "id_recommendation" INTEGER NOT NULL,
    "id_pattern" INTEGER NOT NULL,
    "result" INTEGER NOT NULL,
    CONSTRAINT "fk_recommendation_pattern_recommendation" 
        FOREIGN KEY ("id_recommendation") 
        REFERENCES "recommendation" ("id"),
    CONSTRAINT "fk_recommendation_pattern_pattern" 
        FOREIGN KEY ("id_pattern") 
        REFERENCES "pattern" ("id")
);

CREATE TABLE "metric" (
    "id" INTEGER,
    "timestamp" DATE DEFAULT (datetime('now','localtime')),
    "pid" INTEGER,
    "code_filename" CHAR(1024),
    "code_line_number" INTEGER,
    "code_type" INTEGER,
    "code_extra_info" CHAR(1024),
    "code_language" INTEGER,
    "code_totalruntime" FLOAT,
    "code_totalwalltime" FLOAT,
    "code_importance" FLOAT,
    "code_loopdepth" INTEGER,
    "code_function_name" CHAR(1024),
    "perfexpert_overall" FLOAT,
    "perfexpert_data_accesses_overall" FLOAT,
    "perfexpert_data_accesses_L1d_hits" FLOAT,
    "perfexpert_data_accesses_L2d_hits" FLOAT,
    "perfexpert_data_accesses_L3d_hits" FLOAT,
    "perfexpert_data_accesses_LLC_misses" FLOAT,
    "perfexpert_ratio_floating_point" FLOAT,
    "perfexpert_ratio_data_accesses" FLOAT,
    "perfexpert_instruction_accesses_overall" FLOAT,
    "perfexpert_instruction_accesses_L1i_hits" FLOAT,
    "perfexpert_instruction_accesses_L2i_hits" FLOAT,
    "perfexpert_instruction_accesses_L2i_misses" FLOAT,
    "perfexpert_instruction_TLB_overall" FLOAT,
    "perfexpert_data_TLB_overall" FLOAT,
    "perfexpert_branch_instructions_overall" FLOAT,
    "perfexpert_branch_instructions_correctly_predicted" FLOAT,
    "perfexpert_branch_instructions_mispredicted" FLOAT,
    "perfexpert_floating_point_instr_overall" FLOAT,
    "perfexpert_floating_point_instr_fast_FP_instr" FLOAT,
    "perfexpert_floating_point_instr_slow_FP_instr" FLOAT,
    "perfexpert_GFLOPS____max__overall" FLOAT,
    "perfexpert_GFLOPS____max__packed" FLOAT,
    "perfexpert_GFLOPS____max__scalar" FLOAT,
    "papi_PAPI_TOT_INS" INTEGER,
    "papi_PAPI_TOT_CYC" INTEGER,
    "papi_PAPI_FP_INS" INTEGER,
    "papi_PAPI_L1_ICA" INTEGER,
    "papi_PAPI_L2_ICA" INTEGER,
    "papi_PAPI_L1_DCA" INTEGER,
    "papi_PAPI_L2_DCA" INTEGER,
    "papi_PAPI_L2_ICM" INTEGER,
    "papi_PAPI_L2_TCM" INTEGER,
    "papi_PAPI_L3_TCM" INTEGER,
    "papi_PAPI_L3_TCA" INTEGER,
    "papi_PAPI_TLB_IM" INTEGER,
    "papi_PAPI_TLB_DM" INTEGER,
    "papi_PAPI_BR_INS" INTEGER,
    "papi_PAPI_BR_MSP" INTEGER,
    "papi_PAPI_LD_INS" INTEGER,
    "papi_PAPI_FDV_INS" INTEGER,
    "papi_ITLB_MISSES_WALK_DURATION" INTEGER,
    "papi_DTLB_LOAD_MISSES_WALK_DURATION" INTEGER,
    "papi_SSEX_UOPS_RETIRED_PACKED_SINGLE" INTEGER,
    "papi_SSEX_UOPS_RETIRED_PACKED_DOUBLE" INTEGER,
    "papi_SSEX_UOPS_RETIRED_SCALAR_DOUBLE_SCALAR_SINGLE" INTEGER,
    "papi_FP_COMP_OPS_EXE_SSE_DOUBLE_PRECISION_SSE_FP_SSE_FP_PACKED_SSE_FP_SCALAR_SSE_SINGLE_PRECISION_X87" INTEGER,
    "papi_FP_COMP_OPS_EXE_SSE_FP_SCALAR_SINGLE_SSE_SCALAR_DOUBLE" INTEGER,
    "papi_FP_COMP_OPS_EXE_SSE_FP_PACKED_DOUBLE" INTEGER,
    "papi_FP_COMP_OPS_EXE_SSE_PACKED_SINGLE" INTEGER,
    "papi_DTLB_LOAD_MISSES_CAUSES_A_WALK" INTEGER,
    "papi_DTLB_STORE_MISSES_CAUSES_A_WALK" INTEGER,
    "papi_SIMD_FP_256_PACKED_SINGLE" INTEGER,
    "papi_SIMD_FP_256_PACKED_DOUBLE" INTEGER,
    "papi_ICACHE" INTEGER,
    "papi_ARITH_CYCLES_DIV_BUSY" INTEGER,
    "hound_CPU_freq" FLOAT,
    "hound_CPI_threshold" FLOAT,
    "hound_L1_ilat" FLOAT,
    "hound_L1_dlat" FLOAT,
    "hound_L2_lat" FLOAT,
    "hound_L3_lat" FLOAT,
    "hound_mem_lat" FLOAT,
    "hound_BR_lat" FLOAT,
    "hound_BR_miss_lat" FLOAT,
    "hound_FP_slow_lat" FLOAT,
    "hound_FP_lat" FLOAT,
    "hound_TLB_lat" FLOAT,
    PRIMARY KEY ("id") 
);

--
-- Populate tables
--
BEGIN TRANSACTION;

INSERT INTO [language] ([id], [short], [desc]) VALUES 
    (1, 'c', 'ANSI C, C90, C99, ...');
INSERT INTO [language] ([id], [short], [desc]) VALUES 
    (2, 'fortran', 'Fortran 90, Fortran 95, ...');

INSERT INTO [architecture] ([id], [short], [desc]) VALUES 
    (1, 'x86_64', '64 bit extension of the x86 instruction set');

INSERT INTO [category] ([id], [short], [desc]) VALUES
    (1, 'd-l1', 'bad L1 data access');
INSERT INTO [category] ([id], [short], [desc]) VALUES
    (2, 'd-l2', 'bad L2 data access');
INSERT INTO [category] ([id], [short], [desc]) VALUES
    (3, 'd-mem', 'bad memory access');
INSERT INTO [category] ([id], [short], [desc]) VALUES 
    (4, 'd-tlb', 'bad data TLB access');
INSERT INTO [category] ([id], [short], [desc]) VALUES 
    (5, 'i-access', 'bad instruction access');
INSERT INTO [category] ([id], [short], [desc]) VALUES 
    (6, 'i-tlb', 'bad instruction TLB access');
INSERT INTO [category] ([id], [short], [desc]) VALUES 
    (7, 'br-i', 'bad branch instructions');
INSERT INTO [category] ([id], [short], [desc]) VALUES 
    (8, 'fpt-fast', 'bad float point instuctions (fast FP)');
INSERT INTO [category] ([id], [short], [desc]) VALUES 
    (9, 'fpt-slow', 'bad float point instuctions (slow FP)');

INSERT INTO [recommendation] ([id], [desc], [reason], [example], [loop]) VALUES (1,
    'eliminate common subexpressions involving memory accesses',
    'this optimization reduces the number of executed (slow) memory accesses',
    'd[i] = a * b[i] + c[i];
y[i] = a * b[i] + x[i];
 =====>
temp = a * b[i];
d[i] = temp + c[i];
y[i] = temp + x[i];', null);

INSERT INTO [recommendation] ([id], [desc], [reason], [example], [loop]) VALUES (2,
    'eliminate floating-point operations through distributivity',
    'this optimization reduces the number of executed floating-point operations',
    'd[i] = a[i] * b[i] + a[i] * c[i];
 =====>
d[i] = a[i] * (b[i] + c[i]);', null);

INSERT INTO [recommendation] ([id], [desc], [reason], [example], [loop]) VALUES (3,
    'eliminate floating-point operations through associativity',
    'this optimization reduces the number of executed floating-point operations',
    'd[i] = a[i] * b[i] * c[i];
y[i] = x[i] * a[i] * b[i];
 =====>
temp = a[i] * b[i];
d[i] = temp * c[i];
y[i] = x[i] * temp;', null);

INSERT INTO [recommendation] ([id], [desc], [reason], [example], [loop]) VALUES (4,
    'move loop invariant computations out of loop', 
    'this optimization reduces the number of executed floating-point operations', 
    'loop i {
  x = x + a * b * c[i];
}
 =====>
temp = a * b;
loop i {
  x = x + temp * c[i];
}', 1);

INSERT INTO [recommendation] ([id], [desc], [reason], [example], [loop]) VALUES (5,
    'use float instead of double data type if loss of precision is acceptable',
    'this optimization reduces the amount of memory required to store floating-point data, which often makes accessing the data faster',
'double a[n];
 =====>
float a[n];', null);

INSERT INTO [recommendation] ([id], [desc], [reason], [example], [loop]) VALUES (6,
    'compare squared values instead of computing the square root',
    'this optimization replaces a slow operation with equivalent but much faster operations', 
    'if (x < sqrt(y)) {...}
 =====>
if ((x < 0.0) || (x*x < y)) {...}', null);

INSERT INTO [recommendation] ([id], [desc], [reason], [example], [loop]) VALUES (7,
    'compute the reciprocal outside of loop and use multiplication inside the loop',
    'this optimization replaces many slow operations with one slow and many fast operations that accomplish the same', 
    'loop i {
  a[i] = b[i] / c;
}
 =====>
cinv = 1.0 / c;
loop i {
  a[i] = b[i] * cinv;
}', 1);

INSERT INTO [recommendation] ([id], [desc], [reason], [example], [loop]) VALUES (8,
    'accumulate and then normalize instead of normalizing each element', 
    'this optimization replaces many slow operations with a single slow operation that accomplishes the same', 
    'loop i {
  x = x + a[i] / b;
}
 =====>
loop i {
  x = x + a[i];
}
x = x / b;', 1);

INSERT INTO [recommendation] ([id], [desc], [reason], [example], [loop]) VALUES (9,
    'use trivial assignments inside THEN/ELSE to allow the use of conditional moves',
    'this optimization may allow the compiler to replace a code sequence with an equivalent but faster code sequence that uses no branches',
    'if (x < y)
  a = x + y;
 =====>
temp = x + y;
if (x < y)
  a = temp;', null);

INSERT INTO [recommendation] ([id], [desc], [reason], [example], [loop]) VALUES (10, 
    'express Boolean logic in form of integer computation', 
    'this optimization replaces a code sequence with an equivalent code sequence that is faster because fewer branches are executed', 
    'if ((a == 0) && (b == 0) && (c == 0)) {...}
 =====>
if ((a | b | c) == 0) {...}', null);

INSERT INTO [recommendation] ([id], [desc], [reason], [example], [loop]) VALUES (11,
    'remove IF', 
    'this optimization replaces a code sequence with an equivalent code sequence that may be faster because no branches are needed',
    '/* x is 0 or -1 */
if (x == 0)
  a = b;
else
  a = c;
 =====>
a = (b & ~x) | (c & x);', null);

INSERT INTO [recommendation] ([id], [desc], [reason], [example], [loop]) VALUES (12,
    'unroll loops (more)',
    'this optimization replaces a code sequence with an equivalent code sequence that is faster because fewer branches are executed',
    'loop i {
  a[i] = a[i] * b[i];
}
 =====>
loop i step 2 {
  a[i] = a[i] * b[i];
  a[i+1] = a[i+1] * b[i+1];
}', 1);

INSERT INTO [recommendation] ([id], [desc], [reason], [example], [loop]) VALUES (13,
    'move a loop around a subroutine call into the subroutine', 
    'this optimization replaces a code sequence with an equivalent code sequence that is faster because fewer calls are executed', 
    'f(x) {...x...};
loop i {
  f(a[i]);
}
 =====>
f(x[]) {
  loop j {
    ...x[j]...
  }
};
f(a);', 1);

INSERT INTO [recommendation] ([id], [desc], [reason], [example], [loop]) VALUES (14,
    'move loop invariant tests out of loop', 
    'this optimization replaces a code sequence with an equivalent code sequence that is faster because fewer branches are executed', 
    'loop i {
  if (x < y)
    a[i] = x * b[i];
  else
    a[i] = y * b[i];
}
 =====>
if (x < y) {
  loop i {
    a[i] = x * b[i];
  }
} else {
  loop i {
    a[i] = y * b[i];
  }
}', 1);

INSERT INTO [recommendation] ([id], [desc], [reason], [example], [loop]) VALUES (15,
    'special-case the most often used loop count(s)', 
    'this optimization replaces a code sequence with an equivalent sequence that is faster because fixed loop boundaries often enable better compiler optimizations', 
    'for (i = 0; i < n; i++) {...}
 =====>
if (n == 4) {
  for (i = 0; i < 4; i++) {...}
} else {
  for (i = 0; i < n; i++) {...}
}', 1);

INSERT INTO [recommendation] ([id], [desc], [reason], [example], [loop]) VALUES (16,
    'use inlining', 
    'this optimization replaces a code sequence with an equivalent code sequence that is faster because fewer control transfers are executed', 
    'float f(float x) {
  return x * x;
}
z = f(y);
 =====>
z = y * y;', null);

INSERT INTO [recommendation] ([id], [desc], [reason], [example], [loop]) VALUES (17,
    'change the order of subroutine calls', 
    'this optimization, when allowed, may yield faster execution when it results in more opportunity for compiler optimizations', 
    'f(); h();
 =====>
h(); f();', null);

INSERT INTO [recommendation] ([id], [desc], [reason], [example], [loop]) VALUES (18,
    'factor out sequences of common code into subroutines', 
    'this optimization reduces the code size, which may improve the instruction cache performance', 
    'same_code;
same_code;
 =====>
void f() {same_code;}
f();
f();', null);

INSERT INTO [recommendation] ([id], [desc], [reason], [example], [loop]) VALUES (19,
    'sort subroutines by call chains (subroutine coloring)', 
    'this optimization moves functions to potentially better starting addresses in memory', 
    'f() {...}
g() {...}
h() {...}
loop {
  f();
  h();
}
 =====>
g() {...}
f() {...}
h() {...}
loop {
  f();
  h();
}', null);

INSERT INTO [recommendation] ([id], [desc], [reason], [example], [loop]) VALUES (20,
    'lower the loop unroll factor', 
    'this optimization reduces the code size, which may improve the instruction cache performance', 
    'loop i step 4 {
  code_i;
  code_i+1;
  code_i+2;
  code_i+3;
}
 =====>
loop i step 2 {
  code_i;
  code_i+1;
}', 1);

INSERT INTO [recommendation] ([id], [desc], [reason], [example], [loop]) VALUES (21,
    'make subroutines more general and use them more', 
    'this optimization reduces the code size, which may improve the instruction cache performance', 
    'void f() {
  statements1;
  statementsX;
}
void g() {
  statements2;
  statementsX;
}
 =====>
void fg(int flag) {
  if (flag) {
    statements1;
  } else {
    statements2;
  }
  statementsX;
}', null);

INSERT INTO [recommendation] ([id], [desc], [reason], [example], [loop]) VALUES (22,
    'split off cold code into separate subroutines and place them at end of file',
    'this optimization separates rarely from frequently used code, which may improve the instruction cache performance', 
    'if (unlikely_condition) {
  lots_of_code
}
 =====>
void f() {lots_of_code}
...
if (unlikely_condition)
  f();', null);

INSERT INTO [recommendation] ([id], [desc], [reason], [example], [loop]) VALUES (23,
    'use trace scheduling to reduce the branch taken frequency', 
    'this optimization replaces a code sequence with an equivalent code sequence that may be faster because it reduces (taken) branches and may enable better compiler optimizations', 
    'if (likely_condition)
  f();
else
  g();
h();
 =====>
if (!likely_condition) {
  g(); h();
} else {
  f(); h();
}', null);

INSERT INTO [recommendation] ([id], [desc], [reason], [example], [loop]) VALUES (24,
    'move loop invariant memory accesses out of loop', 
    'this optimization reduces the number of executed (slow) memory accesses', 
    'loop i {
  a[i] = b[i] * c[j]
}
 =====>
temp = c[j];
loop i {
  a[i] = b[i] * temp;
}', 1);

INSERT INTO [recommendation] ([id], [desc], [reason], [example], [loop]) VALUES (25,
    'compute values rather than loading them if doable with few operations', 
    'this optimization replaces (slow) memory accesses with equivalent but faster computations', 
    'loop i {
  t[i] = a[i] * 0.5;
}
loop i {
  a[i] = c[i] - t[i];
}
 =====>
loop i {
  a[i] = c[i] - (a[i] * 0.5);
}', 1);

INSERT INTO [recommendation] ([id], [desc], [reason], [example], [loop]) VALUES (26,
    'enable the use of vector instructions to transfer more data per access', 
    'this optimization increases the memory bandwidth', 
    'align arrays, use only stride-one accesses, make loop count even (pad arrays)
struct {
  double a, b;
} s[127];
for (i = 0; i < 127; i++) {
  s[i].a = 0;
  s[i].b = 0;
}
 =====>
__declspec(align(16)) double a[128], b[128];
for (i = 0; i < 128; i++) {
  a[i] = 0;
  b[i] = 0;
}', 1);

INSERT INTO [recommendation] ([id], [desc], [reason], [example], [loop]) VALUES (27,
    'copy data into local scalar variables and operate on the local copies', 
    'this optimization replaces (slow) memory accesses with equivalent but faster computations', 
    'x = a[i] * a[i];
...
a[i] = x / b;
...
b = a[i] + 1.0;
 =====>
t = a[i];
x = t * t;
...
a[i] = t = x / b;
...
b = t + 1.0;', null);

INSERT INTO [recommendation] ([id], [desc], [reason], [example], [loop]) VALUES (28,
    'align data, especially arrays and structs', 
    'this optimization enables the use of vector instructions, which increase the memory bandwidth', 
    'int x[1024];
 =====>
__declspec(align(16)) int x[1024];', null);

INSERT INTO [recommendation] ([id], [desc], [reason], [example], [loop]) VALUES (29,
    'help the compiler by marking pointers to non-overlapping data with "restrict"',
    'this optimization, when applicable, enables the compiler to tune the code more aggressively',
    'void *a, *b;
 =====>
void * restrict a, * restrict b;', null);

INSERT INTO [recommendation] ([id], [desc], [reason], [example], [loop]) VALUES (30,
    'fuse multiple loops that access the same data',
    'this optimization enables the reuse of loaded data', 
    'loop i {
  a[i] = x[i];
}
loop i {
  b[i] = x[i] - 1;
}
 =====>
loop i {
  a[i] = x[i];
  b[i] = x[i] - 1;
}', 1);

INSERT INTO [recommendation] ([id], [desc], [reason], [example], [loop]) VALUES (31,
    'change the order of loops',
    'this optimization may improve the memory access pattern and make it more cache and TLB friendly', 
    'loop i {
  loop j {...}
}
 =====>
loop j {
  loop i {...}
}', 2);

INSERT INTO [recommendation] ([id], [desc], [reason], [example], [loop]) VALUES (32,
    'employ loop blocking and interchange', 
    'this optimization may improve the memory access pattern and make it more cache and TLB friendly, choose s such that s*s+2*s is less than the cache size', 
    'loop i {
  loop k {
    loop j {
      c[i][j] = c[i][j] + a[i][k] * b[k][j];
    }
  }
}
 =====>
loop k step s {
  loop j step s {
    loop i {
      for (kk = k; kk < k + s; kk++) {
        for (jj = j; jj < j + s; jj++) {
          c[i][jj] = c[i][jj] + a[i][kk] * b[kk][jj];
        }
      }
    }
  }
}', 3);

INSERT INTO [recommendation] ([id], [desc], [reason], [example], [loop]) VALUES (33,
    'unroll outer loop',
    'this optimization may reduce the number of (slow) memory accesses',
    'loop i {
  loop j {
    a[i][j] = b[i][j] * c[j];
  }
}
 =====>
loop i step 4 {
  loop j {
    a[i][j] = b[i][j] * c[j];
    a[i+1][j] = b[i+1][j] * c[j];
    a[i+2][j] = b[i+2][j] * c[j];
    a[i+3][j] = b[i+3][j] * c[j];
  }
}', 2);

INSERT INTO [recommendation] ([id], [desc], [reason], [example], [loop]) VALUES (34,
    'access arrays directly instead of using local copies',
    'this optimization reduces the memory footprint, which may improve cache performance', 
    'loop j {
  a[j] = b[i][j][k];
}
...
loop j {
  ... a[j] ...
}
 =====>
loop j {
  ... b[i][j][k] ...
}', 1);

INSERT INTO [recommendation] ([id], [desc], [reason], [example], [loop]) VALUES (35,
    'reuse temporary arrays for different operations', 
    'this optimization reduces the memory footprint, which may improve cache performance', 
    'loop i {
  t1[i] = ...;
  ... t1[i] ...;
}
...
loop j {
  t2[j] = ...;
  ... t2[j] ...;
}
 =====>
loop i {
  t[i] = ...;
  ... t[i] ...;
}
...
loop j {
  t[j] = ...;
  ... t[j] ...;
}', 1);

INSERT INTO [recommendation] ([id], [desc], [reason], [example], [loop]) VALUES (36,
    'avoid unnecessary array updates', 
    'this optimization reduces the number of memory writes', 
    'loop i {
  a[i] = ...;
  ... a[i] ...
}
// array a[] not read
 =====>
loop i {
  temp = ...;
  ... temp ...
}', 1);

INSERT INTO [recommendation] ([id], [desc], [reason], [example], [loop]) VALUES (37,
    'use smaller types (e.g., float instead of double or short instead of int)',
    'this optimization reduces the memory footprint, which may improve cache performance', 
    'double a[n];
 =====>
float a[n];', null);

INSERT INTO [recommendation] ([id], [desc], [reason], [example], [loop]) VALUES (38,
    'apply loop fission so every loop accesses just a couple of different arrays',
    'this optimization reduces the number of active memory pages, which may improve DRAM performance',
    'loop i {
  a[i] = a[i] * b[i] - c[i];
}
 =====>
loop i {
  a[i] = a[i] * b[i];
}
loop i {
  a[i] = a[i] - c[i];
}', 1);

INSERT INTO [recommendation] ([id], [desc], [reason], [example], [loop]) VALUES (39,
    'componentize important loops by factoring them into their own subroutines',
    'this optimization may allow the compiler to optimize the loop independently and thus tune it better',
    'loop i {...}
...
loop j {...}
 =====>
void li() {loop i {...}}
void lj() {loop j {...}}
...
li();
...
lj();', 1);

INSERT INTO [recommendation] ([id], [desc], [reason], [example], [loop]) VALUES (40,
    'split structs into hot and cold parts, where hot part has pointer to cold part', 
    'this optimization separates rarely and frequently used data, which may improve cache performance', 
    'struct s {
  hot_field;
  many_cold_fields;
} a[n];
 =====>
struct s_hot {
  hot_field;
  struct s_cold *ptr;
} a_hot[n];
struct s_cold {
  many_cold_fields;
} a_cold[n];', null);

INSERT INTO [recommendation] ([id], [desc], [reason], [example], [loop]) VALUES (41,
    'pad memory areas so that temporal elements do not map to same set in cache',
    'this optimization reduces the chance of data overlap in the caches, which may improve cache performance, the final size of each array should be an integer multiple of the cache line size (typically 64 bytes) but should not be a small integer multiple of the cache size',
    'double a[const * cache_size/8], b[const * cache_size/8];
loop i {
  ... a[i] + b[i] ...
}
 =====>
double a[const * cache_size/8 + 8], b[const * cache_size/8 + 8];
loop i {
  ... a[i] + b[i] ...
}', 1);

INSERT INTO [recommendation] ([id], [desc], [reason], [example], [loop]) VALUES (42,
    'allocate an array of elements instead of each element individually',
    'this optimization reduces the memory footprint and enhances spatial locality, which may improve cache performance',
    'loop {
  ... c = malloc(1); ...
}
 =====>
top = n;
loop {
  if (top == n) {
    tmp = malloc(n);
    top = 0;
  }
  ...
  c = &tmp[top++]; ...
}', 1);

INSERT INTO [recommendation_category] ([id_recommendation], [id_category]) VALUES (1, 1);
INSERT INTO [recommendation_category] ([id_recommendation], [id_category]) VALUES (2, 8);
INSERT INTO [recommendation_category] ([id_recommendation], [id_category]) VALUES (2, 9);
INSERT INTO [recommendation_category] ([id_recommendation], [id_category]) VALUES (3, 8);
INSERT INTO [recommendation_category] ([id_recommendation], [id_category]) VALUES (3, 9);
INSERT INTO [recommendation_category] ([id_recommendation], [id_category]) VALUES (4, 8);
INSERT INTO [recommendation_category] ([id_recommendation], [id_category]) VALUES (4, 9);
INSERT INTO [recommendation_category] ([id_recommendation], [id_category]) VALUES (5, 1);
INSERT INTO [recommendation_category] ([id_recommendation], [id_category]) VALUES (5, 9);
INSERT INTO [recommendation_category] ([id_recommendation], [id_category]) VALUES (6, 9);
INSERT INTO [recommendation_category] ([id_recommendation], [id_category]) VALUES (7, 9);
INSERT INTO [recommendation_category] ([id_recommendation], [id_category]) VALUES (8, 9);
INSERT INTO [recommendation_category] ([id_recommendation], [id_category]) VALUES (9, 7);
INSERT INTO [recommendation_category] ([id_recommendation], [id_category]) VALUES (10, 7);
INSERT INTO [recommendation_category] ([id_recommendation], [id_category]) VALUES (11, 7);
INSERT INTO [recommendation_category] ([id_recommendation], [id_category]) VALUES (12, 7);
INSERT INTO [recommendation_category] ([id_recommendation], [id_category]) VALUES (13, 7);
INSERT INTO [recommendation_category] ([id_recommendation], [id_category]) VALUES (14, 7);
INSERT INTO [recommendation_category] ([id_recommendation], [id_category]) VALUES (15, 7);
INSERT INTO [recommendation_category] ([id_recommendation], [id_category]) VALUES (16, 6);
INSERT INTO [recommendation_category] ([id_recommendation], [id_category]) VALUES (17, 6);
INSERT INTO [recommendation_category] ([id_recommendation], [id_category]) VALUES (18, 5);
INSERT INTO [recommendation_category] ([id_recommendation], [id_category]) VALUES (19, 5);
INSERT INTO [recommendation_category] ([id_recommendation], [id_category]) VALUES (19, 6);
INSERT INTO [recommendation_category] ([id_recommendation], [id_category]) VALUES (20, 5);
INSERT INTO [recommendation_category] ([id_recommendation], [id_category]) VALUES (21, 5);
INSERT INTO [recommendation_category] ([id_recommendation], [id_category]) VALUES (22, 5);
INSERT INTO [recommendation_category] ([id_recommendation], [id_category]) VALUES (23, 5);
INSERT INTO [recommendation_category] ([id_recommendation], [id_category]) VALUES (24, 1);
INSERT INTO [recommendation_category] ([id_recommendation], [id_category]) VALUES (25, 2);
INSERT INTO [recommendation_category] ([id_recommendation], [id_category]) VALUES (25, 3);
INSERT INTO [recommendation_category] ([id_recommendation], [id_category]) VALUES (26, 1);
INSERT INTO [recommendation_category] ([id_recommendation], [id_category]) VALUES (27, 1);
INSERT INTO [recommendation_category] ([id_recommendation], [id_category]) VALUES (28, 1);
INSERT INTO [recommendation_category] ([id_recommendation], [id_category]) VALUES (29, 1);
INSERT INTO [recommendation_category] ([id_recommendation], [id_category]) VALUES (30, 2);
INSERT INTO [recommendation_category] ([id_recommendation], [id_category]) VALUES (30, 3);
INSERT INTO [recommendation_category] ([id_recommendation], [id_category]) VALUES (31, 3);
INSERT INTO [recommendation_category] ([id_recommendation], [id_category]) VALUES (31, 4);
INSERT INTO [recommendation_category] ([id_recommendation], [id_category]) VALUES (32, 3);
INSERT INTO [recommendation_category] ([id_recommendation], [id_category]) VALUES (33, 1);
INSERT INTO [recommendation_category] ([id_recommendation], [id_category]) VALUES (34, 2);
INSERT INTO [recommendation_category] ([id_recommendation], [id_category]) VALUES (34, 3);
INSERT INTO [recommendation_category] ([id_recommendation], [id_category]) VALUES (35, 2);
INSERT INTO [recommendation_category] ([id_recommendation], [id_category]) VALUES (35, 3);
INSERT INTO [recommendation_category] ([id_recommendation], [id_category]) VALUES (36, 2);
INSERT INTO [recommendation_category] ([id_recommendation], [id_category]) VALUES (36, 3);
INSERT INTO [recommendation_category] ([id_recommendation], [id_category]) VALUES (37, 3);
INSERT INTO [recommendation_category] ([id_recommendation], [id_category]) VALUES (37, 4);
INSERT INTO [recommendation_category] ([id_recommendation], [id_category]) VALUES (38, 3);
INSERT INTO [recommendation_category] ([id_recommendation], [id_category]) VALUES (39, 1);
INSERT INTO [recommendation_category] ([id_recommendation], [id_category]) VALUES (39, 2);
INSERT INTO [recommendation_category] ([id_recommendation], [id_category]) VALUES (39, 3);
INSERT INTO [recommendation_category] ([id_recommendation], [id_category]) VALUES (40, 3);
INSERT INTO [recommendation_category] ([id_recommendation], [id_category]) VALUES (41, 3);
INSERT INTO [recommendation_category] ([id_recommendation], [id_category]) VALUES (42, 3);

INSERT INTO [pattern] ([id], [language], [recognizer], [desc]) VALUES
    (1, 1, 'nested_c_loops', 'recognize two or more perfect nested loops');

INSERT INTO [transformation] ([id], [language], [transformer], [desc]) VALUES
    (1, 1, 'loop_interchange', 'interchange two perfect nested loops');
INSERT INTO [transformation] ([id], [language], [transformer], [desc]) VALUES
    (2, 1, 'loop_tiling', 'tile two perfect nested loops');

INSERT INTO [recommendation_transformation] ([id_recommendation], [id_transformation]) VALUES (31, 1);
INSERT INTO [recommendation_transformation] ([id_recommendation], [id_transformation]) VALUES (32, 1);
-- we need a recommendation for loop tiling, the one we have is tiling AND interchange

INSERT INTO [transformation_pattern] ([id_transformation], [id_pattern]) VALUES (1, 1);
INSERT INTO [transformation_pattern] ([id_transformation], [id_pattern]) VALUES (2, 1);

--
-- AustoSCOPE recommendation selection algorithm
--
-- @RID is the rowid
-- @LPD is the loop depth
--
INSERT INTO function ([desc], [statement]) VALUES ('AutoSCOPE recommendation algorithm', "SELECT
recom.id AS recommendation_id,
-- normalize representative metrics
-- 0.1 represents the 'MINSUPPORT' constant
SUM(
  (CASE c.short
    WHEN 'd-l1'
    THEN (metric.perfexpert_data_accesses_L1d_hits - (maximum * 0.1))
    ELSE 0 END) +
  (CASE c.short
    WHEN 'd-l2'
    THEN (metric.perfexpert_data_accesses_L2d_hits - (maximum * 0.1))
    ELSE 0 END) +
  (CASE c.short
    WHEN 'd-l3'
    THEN (metric.perfexpert_data_accesses_L3d_hits - (maximum * 0.1))
    ELSE 0 END) +
  (CASE c.short
    WHEN 'd-mem'
    THEN (metric.perfexpert_data_accesses_LLC_misses - (maximum * 0.1))
    ELSE 0 END) +
  (CASE c.short
    WHEN 'd-tlb'
    THEN (metric.perfexpert_data_TLB_overall - (maximum * 0.1))
    ELSE 0 END) +
  (CASE c.short
    WHEN 'i-access'
    THEN (metric.perfexpert_instruction_accesses_overall - (maximum * 0.1))
    ELSE 0 END) +
  (CASE c.short
    WHEN 'i-tlb'
    THEN (metric.perfexpert_instruction_TLB_overall - (maximum * 0.1))
    ELSE 0 END) +
  (CASE c.short
    WHEN 'br-i'
    THEN (metric.perfexpert_branch_instructions_overall - (maximum * 0.1))
    ELSE 0 END) +
  (CASE c.short
    WHEN 'fpt-fast'
    THEN (metric.perfexpert_floating_point_instr_fast_FP_instr - (maximum * 0.1))
    ELSE 0 END) +
  (CASE c.short
    WHEN 'fpt-slow'
    THEN (metric.perfexpert_floating_point_instr_slow_FP_instr - (maximum * 0.1))
    ELSE 0 END)
  ) AS score
FROM recommendation AS recom
INNER JOIN recommendation_category AS rc ON recom.id = rc.id_recommendation
INNER JOIN category AS c ON rc.id_category = c.id
JOIN metric
JOIN
  -- find the maximum between representative metrics
  (SELECT
    MAX(
      metric.perfexpert_data_accesses_L1d_hits,
      metric.perfexpert_data_accesses_L2d_hits,
      metric.perfexpert_data_accesses_L3d_hits,
      metric.perfexpert_data_accesses_LLC_misses,
      metric.perfexpert_data_TLB_overall,
      metric.perfexpert_instruction_accesses_overall,
      metric.perfexpert_instruction_TLB_overall,
      metric.perfexpert_branch_instructions_overall,
      metric.perfexpert_floating_point_instr_fast_FP_instr,
      metric.perfexpert_floating_point_instr_slow_FP_instr
    ) AS maximum
    FROM metric
    WHERE 
      -- limit the result to codes where the overall performance is higher than 1
      -- 0.5 and 1.0 represents 'good-int-CPI' and 'good-fp-CPI
      metric.perfexpert_overall * 100 / (0.5 *
        (100 - metric.perfexpert_ratio_floating_point) + 1.0 *
        metric.perfexpert_ratio_floating_point) > 1
    AND
      -- Recommender rowid, the last inserted bottleneck
      metric.id = @RID
  )
WHERE (recom.loop <= @LPD AND metric.code_type = 3) OR
      (recom.loop IS NULL AND metric.code_type = 2)
  -- Recommender rowid, the last inserted bottleneck
  AND metric.id = @RID
GROUP BY recom.id
-- ranked by their scores
ORDER BY score DESC;");

INSERT INTO function ([desc], [statement]) VALUES ('Patata',
"SELECT 20 AS recommendation_id, 1.1 AS score;");

END TRANSACTION;