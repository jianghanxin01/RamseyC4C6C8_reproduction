# Complete C++14 input reconstruction

`RamseyInputs.exe` reconstructs the graph-to-CNF inputs of all nine revision 53 instances. It compares every clause, including all auxiliary clauses, with the supplied CNF. It does not call Python, a SAT solver, or any third-party library.

This is an input audit, not a proof of unsatisfiability. The separate RUP checker must also accept each certificate. The mathematical reductions covering the nine cases remain ordinary proofs in the paper; this program does not formalize them.

## Run the supplied executable

The following commands assume the integrated distribution has `bin` and `data` folders. Open PowerShell in that distribution folder.

```powershell
.\bin\RamseyInputs.exe --all --data .\data
```

The expected final line is `ALL REQUESTED INPUT CHECKS PASSED`, with exit code 0. Each of the nine cases must say `PASS`. A typical run on the development machine took about ten seconds; this is not a time limit.

To save a machine-readable report and produce fresh CNFs:

```powershell
.\bin\RamseyInputs.exe --all --data .\data --generate .\fresh_inputs_1 --report .\input_report_1.json
```

All output files must be new. To repeat this command, choose another output directory/report name. Existing files, including distributed certificates, are never overwritten. The report's parent directory must already exist.

To audit just one or several cases:

```powershell
.\bin\RamseyInputs.exe --case B-root-1 --case B-root-2 --data .\data
.\bin\RamseyInputs.exe --list
```

To generate CNFs without reading the supplied CNFs:

```powershell
.\bin\RamseyInputs.exe --all --generate .\fresh_inputs_2 --generate-only --report .\generation_report_2.json
```

This last command reports `GENERATED_NOT_COMPARED`, not an input audit pass. Its outputs can be passed to a SAT solver. The program itself performs no SAT search.

## Files, identities, and comparison

| Case ID | Supplied CNF relative to `--data` | Variables | Clauses |
|---|---|---:|---:|
| `48T1` | `arrows/certificates/48T1.cnf` | 31 | 5,135 |
| `48T2` | `arrows/certificates/48T2.cnf` | 31 | 4,567 |
| `H10-I3` | `hexagon/hexagon_refined52/I3.cnf` | 521 | 17,903 |
| `H10-K2_K1` | `hexagon/hexagon_refined52/K2_K1.cnf` | 521 | 17,903 |
| `H10-P3` | `hexagon/hexagon_refined52/P3.cnf` | 521 | 17,903 |
| `B-root-1` | `blue/blue_d1_full53.cnf` | 1,672 | 453,834 |
| `B-root-2` | `blue/blue_d2_full53.cnf` | 1,553 | 462,022 |
| `B-leafK4` | `blue/blue_high_leafK4/instance.cnf` | 1,335 | 35,107 |
| `B-C9` | `blue/blue_high_C9/instance.cnf` | 1,085 | 362,228 |

Generated names are flat: `48T1.cnf`, `H10-I3.cnf`, `B-root-1.cnf`, and so on. They retain the original variable numbering and clause multiplicities. Clauses and their literals are sorted before writing/comparison. Thus their byte hashes can differ from the original CNFs. The original CNF/RUP pairs must retain their published hashes; input equivalence is established by complete clause-multiset equality, not by matching the hashes of newly generated files.

Fixed units are simplified during construction exactly in the families specified in `INPUT_SEMANTICS.md`. This is not a general equisatisfiability checker accepting arbitrary encodings: it reconstructs one completely specified encoding of each graph problem.

The DIMACS reader rejects missing/wrong/duplicate headers, incorrect counts, out-of-range variables, malformed tokens, unterminated clauses, repeated literals and tautological clauses. Repeated *whole clauses* are retained and compared with multiplicity. Clause order, literal order, harmless whitespace and standard comment lines may vary.

Exit codes: 0 = all selected operations succeeded; 1 = at least one case failed; 2 = a command-line, general file-writing, or top-level error. A report contains per-case statuses, dimensions, clause-family counts and elapsed time. No result should be inferred from a partial log after a nonzero exit.

## Build from source with Visual Studio

Install the Visual Studio C++ desktop workload and Windows SDK. Either double-click `BuildRelease.cmd`, run it from PowerShell, or open `RamseyInputs.vcxproj` and choose **Release / x64**. The project uses the installed VS 2026 `v145` toolset. The command script discovers an installed Visual Studio x64 compiler and does not require a particular toolset number.

The output is `bin\x64\Release\RamseyInputs.exe` below the source directory. The script uses `/std:c++14 /O2 /MT /EHsc /W4 /WX`; `/MT` links the C++ runtime statically. Python is not required for building or running this component.

From an x64 Native Tools command prompt the essential build command is:

```text
cl /std:c++14 /O2 /MT /EHsc /W4 /WX /utf-8 /DWIN32_LEAN_AND_MEAN /DNOMINMAX RamseyInputs.cpp /FeRamseyInputs.exe
```

The source also contains a POSIX entry point and avoids `std::filesystem`. The distributed executable was tested on Windows x64; other platforms require their own compilation and validation.

## Audit boundary

The old revision 53 low-degree input audit covered graph and cardinality clauses but left the auxiliary lexicographic/path blocks to source inspection. This revision reconstructs those blocks completely. Nevertheless, equality of the intended CNF and the supplied CNF, RUP proof checking, and the paper's case-coverage arguments are three distinct obligations. External extremal values cited from McKay's tables are not certified by these nine inputs.
