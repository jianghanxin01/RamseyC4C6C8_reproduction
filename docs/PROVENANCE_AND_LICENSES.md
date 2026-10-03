# Source provenance, adaptations and licensing

## Native project programs

`RamseyInputs.cpp` completely reconstructs the nine specific graph-to-CNF
encodings, including all auxiliary clauses. It uses the C++14 standard library
and small platform path helpers, and incorporates no external SAT solver.
`RamseyRup.cpp` is a C++14 implementation of the watched-literal RUP-checking
algorithm used in the project's earlier C# checker, with stricter parsing and
an explicit final-empty requirement. It likewise incorporates no solver code.
The Windows launchers orchestrate these programs and do no mathematical search.

These project sources and explanatory material were prepared with OpenAI Codex
assistance. No claim of independent human authorship of the different programs
is made. The earlier checker and the input-reconstruction source had no explicit
project licence; this package does not invent an owner or assert that those
files are public domain or covered by a third party's licence. The accompanying
paper states the authors' responsibility and AI use. The new solver wrapper
has the explicit MIT notice preserved in `src/solver/WRAPPER_LICENSE.txt`.

## Official SAT solver

The SAT algorithm comes from Glucose and MiniSat, with all copied notices
retained. Relevant original sources are:

- Official repository: https://github.com/audemard/glucose
- Release: https://github.com/audemard/glucose/releases/tag/4.2.1
- Release commit prefix: `084d737`.
- Original archive: https://codeload.github.com/audemard/glucose/zip/refs/tags/4.2.1
- Distributed archive: `vendor/original_glucose_4.2.1.zip`, **135,863 bytes**.
- Archive SHA256: `71f5a6b11757bf50b2774fcceafa5e298e864cdbc9b489d115ea9206ba6bb519`.

The required sequential-core sources are supplied under `src/solver/upstream/`.
The original `LICENSE`, `README.md`, `CHANGELOG` and notices in all copied files
are preserved. They remain distinct from the wrapper's MIT notice, including
any historical conditions in the original notices. This package does not
relicense the upstream solver or claim its algorithm as new project work.

Exactly four copied upstream files were adapted:

| File | Adaptation |
|---|---|
| `core/Solver.cc` | Omit unused `SimpSolver.h`; replace `putc_unlocked` by `putc`; guard the final empty-clause/write-close block so the wrapper owns final emission and checked close. |
| `core/SolverTypes.h` | Omit unused `pthread.h`. |
| `utils/ParseUtils.h` | Exclude compressed-stream/ZLIB helpers under `RAMSEY_NO_ZLIB`; retain string parsing helpers. |
| `utils/System.h` | Supply Windows `realTime()` with `std::chrono::steady_clock`. |

`src/solver/windows_adaptations.patch` records the source changes with relative
`a/` and `b/` paths. `src/solver/source_manifest.json` records original and
adapted hashes for every copied upstream file. No branching, propagation,
learning, restart, minimization or certificate-inference algorithm was changed.
The wrapper uses the core `Solver` and no separate preprocessing executable.

## Distributed inputs and evidence

The 18 data files are byte-for-byte copies of the nine original CNF/RUP pairs
from the preceding proof archive. Stable IDs identify instances, not manuscript
version numbers. `instances.json` records their exact identities and hashes.
The C++ input program reconstructs every clause family, including the low
blue path and lexicographic auxiliary blocks that previously had only source
inspection. Historical checker runs validated all nine archived certificates.

Recorded reports have been sanitized to use relative distribution paths or
relative historical labels. They retain numerical results, counts, timings and
hashes; full raw reports remain in the development records. A historical label
such as `recorded-runs/full-regeneration/generated/B-root-1.rup` identifies a
recorded experiment, not an additional file promised inside this distribution.
The newly generated large traces are omitted because the archived traces and
reproduction programs already supply the corresponding verification/reproduction
routes. See `reports/README.txt` for the exact evidence scope.

The recorded historical solver hash identifies the earlier tested build.
The solver supplied in RamseyC4C6C8 was compiled again. Its executable identity
and the checks performed for this release are recorded separately in
`reports/release_validation.json`.

`SHA256SUMS.txt` identifies immutable distributed files. Hash agreement is a
file-integrity check, not proof of the encoded graph statement. Reconstructed
input semantics, accepted RUP certificates, and the mathematical case-coverage
arguments are separate obligations. None of these records is described as a
Lean or other formal verification of the entire program or Ramsey argument.
