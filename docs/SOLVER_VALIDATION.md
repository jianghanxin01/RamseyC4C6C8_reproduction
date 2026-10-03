# Native solver validation, 23 September 2026

This is a historical validation record. The results and executable hash below
refer to the build tested on that date. The solver supplied in RamseyC4C6C8 was
compiled again; its identity and the checks performed for this release are
recorded separately in `../reports/release_validation.json`.

**All nine original manuscript formulas were solved again, and every newly generated proof passed the independent native RUP checker.** The original input SHA-256 matched its registered value in every case. The old proof files were not used to generate these traces.

Build: Microsoft Visual Studio 2026 v18.10.1, x64 tools 14.51.36257.0; C++14, optimized Release, `/MT` static C/C++ runtime. Runtime dependencies from `dumpbin`: `KERNEL32.dll` only.

Solver executable SHA-256: `62ee399c1a46b0aee54247574e26bbaad02e8f2b0b7620c0ee4f3acf5ddb146c`.

| Original input | Solve seconds | Check seconds | New proof bytes | Result |
|---|---:|---:|---:|---|
| B-root-1 | 13.112 | 107.993 | 12,880,205 | UNSAT; VERIFIED |
| B-root-2 | 14.595 | 116.484 | 7,806,462 | UNSAT; VERIFIED |
| B-leafK4 | 0.140 | 0.158 | 21,682 | UNSAT; VERIFIED |
| B-C9 | 0.966 | 1.171 | 31,575 | UNSAT; VERIFIED |
| H10-I3 | 0.177 | 0.701 | 867,750 | UNSAT; VERIFIED |
| H10-K2_K1 | 0.138 | 0.467 | 658,968 | UNSAT; VERIFIED |
| H10-P3 | 0.152 | 0.489 | 672,919 | UNSAT; VERIFIED |
| 48T1 | 0.300 | 1.072 | 959,731 | UNSAT; VERIFIED |
| 48T2 | 0.271 | 0.993 | 904,447 | UNSAT; VERIFIED |
| **Total** | **29.852** | **229.528** | **24,803,739** | **9/9** |

Every solver exit was 20; every checker exit was 0. Each trace ends with a checked empty-clause addition. Full-precision timings, CNF/proof SHA-256 values, executable/checker hashes and logs are retained in `../reports/solver_original_all_nine.json` (with raw logs retained in development records). The traces are new: their size, sequence and hashes are allowed to differ from previously archived valid certificates.

The wrapper also passed **78 smaller tests**, including 60 generated formulas independently classified by exhaustive truth-table evaluation, plus parsing failures and file-preservation checks. The independent checker verified every UNSAT trace in that test set. The sanitized results are in `../reports/solver_wrapper_tests.json`; the original development script and raw records are retained separately.

These software tests do not replace mathematical validation. Correctness of a
given computation still rests on both the encoded-input semantics and a valid
proof checked against that exact input.
