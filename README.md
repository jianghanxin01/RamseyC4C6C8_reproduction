# Reproduction package v2 for R(C4,C6,C8)=12

This is the compact public computational archive accompanying the paper and
its mathematical proof supplement. It contains **nine CNF formulas and nine
RUP certificates** in three modules, with complete C++14 source for input
reconstruction, proof generation, and independent RUP checking. Version 2
repackages the same 18 formula/certificate files and the same three Release
executables as the previous package; it removes historical logs and duplicate
guides. No new computational assertion is introduced by the repackaging.

## 一键运行 / One-click run

在 Windows x64 上完整解压 ZIP 到可写目录，然后双击
`ONE_CLICK_REPRODUCE_V2.cmd`。默认依次校验文件哈希、核验九份原始证明、
重新生成九份输入和证明并核验新证明。运行结果写入新建的 `results/` 目录。
运行所附 EXE 不需要 Visual Studio、Python 或另装 SAT 求解器。

On Windows x64, extract the *entire* ZIP to a writable folder and double-click
`ONE_CLICK_REPRODUCE_V2.cmd`. The default command checks file hashes, checks
all nine archived proofs, reconstructs all nine inputs, generates nine new
proofs, and checks each new proof against its newly generated input. It writes
logs and new files to fresh `results/` subdirectories. Paths containing spaces
are supported. The supplied executables need no Python, Visual Studio, or
separate SAT installation. Windows PowerShell is used for the file-hash check.

For a quick test from a command prompt, while still auditing all nine inputs:

```text
ONE_CLICK_REPRODUCE_V2.cmd --no-pause --case B-leafK4
```

For the complete run without a final pause:

```text
ONE_CLICK_REPRODUCE_V2.cmd --no-pause
```

The launcher returns 0 only when every requested step succeeds. A solver's
`UNSAT` message is insufficient: `RamseyRup.exe` must check the corresponding
proof and accept its final empty-clause addition. The complete run may take
several minutes; there is no fixed runtime guarantee.

## The nine cases

| Module | Stable instance IDs | Mathematical role |
|---|---|---|
| Arrow graphs | `48T1`, `48T2` | Two local `(C4,C8)` arrow relations |
| Ten-vertex hexagon cases | `H10-I3`, `H10-K2_K1`, `H10-P3` | A small `C6`-free extremal statement after the paper's hand reduction |
| Twelve-vertex octagon cases | `B-root-1`, `B-root-2`, `B-leafK4`, `B-C9` | Four normalized cases in the `C8`-free argument |

`instances.json` lists the exact file paths and SHA-256 hashes for all 18
data files. `docs/INPUT_SEMANTICS.md` explains the graph interpretation,
normalizations, auxiliary variables, and clause families. The paper and proof
supplement establish why these nine cases cover the mathematical claims.
The archive does not encode a full three-colouring search of `K12`, certify
external extremal data, or formalize the entire proof.

## Files and source build

- `data/`: the nine CNF/RUP pairs; `bin/`: three Windows x64 Release programs.
- `src/inputs/`: C++14 graph-input constructor; `src/checker/`: C++14 RUP
  checker; `src/solver/`: C++14 solver wrapper and adapted Glucose core.
- `vendor/`: original Glucose 4.2.1 archive for source comparison;
  `docs/PROVENANCE_AND_LICENSES.md`: changes, authorship, and notices.
- `CHECK_HASHES.cmd`, `VERIFY_ALL.cmd`, `REPRODUCE_ALL.cmd`, and `BUILD_ALL.cmd`:
  separate integrity, checking, regeneration, and source-build commands.

The distributed programs use optimized C++14 Release builds with a static
C/C++ runtime. To rebuild them, install the Visual Studio Desktop development
with C++ workload and Windows SDK, then run `BUILD_ALL.cmd`. It creates
`rebuilt_bin/` without overwriting the distributed executables. Select those
programs with `VERIFY_ALL.cmd --no-pause --bin rebuilt_bin` or
`REPRODUCE_ALL.cmd --no-pause --bin rebuilt_bin`.

`SHA256SUMS.txt` identifies every immutable distributed file except itself.
Run `CHECK_HASHES.cmd --no-pause` to compare the files in an extracted package.
The archive's SHA-256 should also be recorded on its public deposit page.
Hashes identify bytes; mathematical correctness also depends on the encoding
semantics, accepted RUP derivations, and the hand arguments in the paper.
Generated `results/` and `rebuilt_bin/` are deliberately outside the manifest.

The materials were prepared with OpenAI Codex assistance. The programs and
checker are not formally verified; the paper contains the full AI-use
statement and author responsibility declaration.
