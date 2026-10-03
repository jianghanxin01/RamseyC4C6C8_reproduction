# RamseySolve: Glucose 4.2.1 Windows wrapper

From the package root:

```text
bin\RamseySolve.exe INPUT.cnf NEW_OUTPUT.rup
bin\RamseyRup.exe INPUT.cnf NEW_OUTPUT.rup
```

Solver exit statuses are **20 for UNSAT**, **10 for SAT**, and **2 for an error**.
After solver status 20, the independent checker must still print `VERIFIED`
and return 0 against that exact input. `NEW_OUTPUT.rup` must be a new file.
The wrapper uses Windows `CREATE_NEW`, so an existing proof or input is not
truncated. Unicode paths and spaces are supported. No network is used.

The whole DIMACS input is parsed before the output is opened. Header, literal
ranges, counts and terminators are checked; wrapped and multiple-per-line CNF
clauses are allowed. The wrapper enables text certificate logging before adding
any clause, invokes the official core solver with empty assumptions, emits the
final empty clause only after an UNSAT result, and checks writing, flushing and
closing before returning 20. A SAT result or failed/interrupted run does not
produce an accepted UNSAT certificate. Partial files are not deleted.

The solver is the official Glucose 4.2.1 sequential core; this project did not
invent or rewrite its SAT algorithm. Exactly four upstream files have small
Windows/proof-stream adaptations, detailed in the package's
`docs/PROVENANCE_AND_LICENSES.md`. `windows_adaptations.patch` has relative
paths; `source_manifest.json` identifies original and adapted file hashes.
The original release archive is `../../vendor/original_glucose_4.2.1.zip`.

`build_release.cmd` builds using Visual Studio's x64 compiler with
`/std:c++14 /O2 /MT /EHsc /DNDEBUG`, `RAMSEY_NO_ZLIB` and
`RAMSEY_EXTERNAL_PROOF_STREAM`. The root `BUILD_ALL.cmd` instead builds source
copies under `results/` and places the result in `rebuilt_bin/`. The inspected
Release executable imports only `KERNEL32.dll`. Legacy warnings in the official
solver are retained instead of altering its internal storage or algorithms.

The wrapper passed 78 tests, including 60 small formulas independently
classified by exhaustive truth tables; each UNSAT result was checked separately.
Historical validation (23 September 2026): all nine original formulas were solved again and their new proofs checked.
The final full package workflow additionally solved the nine newly generated,
sorted CNFs and checked their fresh proofs. The sanitized records are
`../../reports/solver_wrapper_tests.json`,
`../../reports/solver_original_all_nine.json`, and
`../../reports/full_regeneration.json`.

`WRAPPER_LICENSE.txt` covers the new wrapper and its scripts. The upstream
files retain their original notices and `upstream/LICENSE`; these separate
notices have not been replaced by the wrapper licence. The requested Glucose
and MiniSat citations belong in the accompanying manuscript.

The RamseyC4C6C8 solver was rebuilt from this source. Its current executable hash and verification results are in `../../reports/release_validation.json`.
