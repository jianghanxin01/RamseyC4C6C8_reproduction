# Complete encoding specification

This document specifies the formulas reconstructed by `RamseyInputs.cpp`. Vertex labels start at zero. It explains the intended interpretation; complete clause-multiset comparison confirms that every supplied clause agrees with this specification's implementation. It does not replace the paper's case-coverage proofs or the separate RUP check.

## Variables and normalization

For a complete graph on `n` vertices, the variable of edge `{a,b}`, `a<b`, is

`a(2n-a-1)/2 + b-a`.

Thus graph variables are 1–45 for `H10` and 1–66 for `B`. In the low blue cases the eleven-vertex graph `H=B-11` retains the twelve-vertex numbering, so its 55 edge variables need not form a consecutive interval. A graph variable is true precisely when its edge is present. For the arrow cases it is true precisely when its host edge is red; false means blue.

Fixed-unit simplification discards a clause containing an already true literal and removes already false literals from other clauses. Fixed units themselves remain in the formula. Simplification is applied only where stated below. Within a clause, literals are sorted. Across clauses, multiplicity is preserved except in the two explicitly specified deduplication stages.

## Cardinality counter: a complete specification and equivalence proof

Let `x_1,...,x_n` be the chosen edge variables, let `L` be the required number of present edges, and put `k=n-L`. Auxiliary state `S(i,j)` is defined on the truncated grid

`1 <= j <= k`, `j <= i <= j+L-1`.

Its intended meaning is “at least `j` of the first `i` edges are absent.” If `t` is the last variable already allocated, the numeric identity is

- `S(j,j) = t+2j-1`;
- `S(j+1,j) = t+2j`;
- `S(j+h,j) = t+kh+j`, for `2 <= h <= L-1`.

This is a bijection to variables `t+1,...,t+kL`. The program writes exactly four families:

1. `x_i OR S(i,1)` for `1 <= i <= L`;
2. `NOT S(i,j) OR S(i+1,j)` for `1 <= j <= k`, `j <= i <= j+L-2`;
3. `x_i OR NOT S(i-1,j-1) OR S(i,j)` for `2 <= j <= k`, `j <= i <= j+L-1`;
4. `x_i OR NOT S(i-1,k)` for `k+1 <= i <= n`.

If at most `k` edges are absent, give each state its intended truth value; all clauses hold. Conversely, suppose absent edges occur at positions `t_1<...<t_(k+1)`. Then `j <= t_j <= L+j-1`. Families 1–3 force `S(t_j,j)` successively, with family 2 propagating between these positions. They then force `S(t_(k+1)-1,k)`, and family 4 contradicts the absent edge at `t_(k+1)`. Therefore an assignment to the graph variables extends to the counter exactly when at least `L` edges are present. No SAT-library cardinality routine is called.

Parameters are `(n,L,t)=(45,17,45)` for H10, `(55,22,66)` for `B-root-1`, `(55,21,66)` for `B-root-2`, and `(66,23,66)` for the two high blue cases. Only the high blue counters are simplified by their fixed graph units.

## Common graph constraints

For a forbidden subgraph, write a negative clause containing every edge of each occurrence. Extra edges on the same vertices are allowed: these are ordinary, not induced, forbidden subgraphs.

Every simple cycle is enumerated with its least vertex first and the second vertex smaller than the last, removing exactly rotation and reversal repetitions. This is not a shortest-cycle search.

For the blue graphs:

- For every disjoint `A,D` with sizes 4 and 5, a positive clause on all 20 cross-edges asserts that the complement contains no `K_(4,5)`.
- For every seven-set, write 21 positive clauses, each omitting one of its 21 potential edges. Their conjunction says that at least two edges are present in the seven-set.
- To assert degree at least `d` at a vertex, for every `(d-1)`-subset of its incident edge variables write a positive clause on the remaining incident edges. This excludes the possibility that all neighbors lie in that subset.

## Two arrow inputs

The first host is `K_(4,3,3)` with parts `0..3`, `4..6`, `7..9`, with edges `{1,5}` and `{6,7}` deleted. The second is `K_(4,4,2)` with parts `0..3`, `4..7`, `8..9`, with edge `{0,7}` deleted. In each case the 31 host edges are numbered in lexicographic endpoint order.

Each host four-cycle yields a negative clause, forbidding an all-red `C4`. Each host eight-cycle yields a positive clause, forbidding an all-blue `C8`. There are no auxiliary variables or symmetry restrictions. The counts are `(131,5004)` for `48T1` and `(139,4428)` for `48T2`.

## Three H10 inputs

All three inputs assert, on ten vertices:

- no `C6`: 12,600 clauses;
- no `K4`: 210 clauses;
- no `W5` (a four-cycle plus a universal hub): 3,780 clauses;
- minimum degree at least 3: 360 clauses;
- at least 17 edges: 941 counter clauses.

Root 0 is adjacent exactly to `1,2,3`; this gives nine unit clauses. Three further units specify their internal edges: none for `I3`, only `{1,2}` for `K2_K1`, and exactly `{1,2},{2,3}` for `P3`. No graph-unit simplification is used in this module. Each formula has 521 variables and 17,903 clauses. The separate hand reduction explains why these three root patterns suffice.

## Low blue inputs and saturation witnesses

Root 11 has neighborhood `P={0,...,d-1}`, for `d=1,2`. Root units fix its eleven incident variables. All eight-cycles inside `H=B-11` are prohibited (415,800 clauses). For `d=2`, every six-edge path from 0 to 1 inside H is prohibited (15,120 clauses), since it would complete an eight-cycle through root 11. For `d=1` no cycle uses the root.

The complement condition, seven-set condition, and degree-at-least-`d` conditions at vertices of H are simplified using root units and deduplicated jointly into a single set of positive clauses. This yields 28,297 clauses for `d=1` and 22,172 for `d=2`. The counter asserts at least `23-d` edges in H, hence at least 23 edges in B.

For each `w` outside P in H, introduce seven positions for a path of length six. Position 0 chooses a vertex of P, position 6 is w, and positions 1–5 choose from `H-w`. Every position has exactly one chosen vertex, and each vertex occurs in at most one position. Consecutive chosen vertices force their graph edge. These clauses are equivalent, after existential quantification over the new variables, to existence of a simple six-edge path from a member of P to w. Numbering is by w, position, then vertex, all increasing.

Such a path is necessary in a C8-saturated graph: adding the missing edge `11w` closes an eight-cycle, and removing its two edges incident with 11 leaves precisely this path in H. We require only this consequence of saturation, not full saturation of every missing edge.

The path blocks have respectively 520 and 477 variables, and 7,160 and 6,579 clauses. The next section specifies the lexicographic blocks added before the path variables.

## Lexicographic restrictions

Fix a total lexicographic order on the complete graph edge-bit string, with true larger than false. Among labelings preserving each specified free cell, a lexicographically maximal labeling exists. For any pair `u<v` in one cell, compare their adjacency rows using increasing common columns and omitting u,v. The row of u must be lexicographically at least that of v: otherwise transposing u and v increases the full edge string. Their shared edge is unchanged; before the first differing row entry, every changed edge entry agrees. Thus all these pairwise restrictions may be imposed simultaneously without losing every labeling of a graph.

The low cases use cells P and `H-P`. Columns are vertices of H other than u,v; the omitted root column agrees within a cell. If the compared bits are a,b, the first comparison clause is `a OR NOT b`; later clauses are guarded by the negation of the preceding prefix flag. Except at the last column a new flag q is introduced, with

`NOT a OR NOT b OR q`, and `a OR b OR q`,

also guarded by the preceding flag when present. Equality forces continuation. At the first strict comparison `(a,b)=(1,0)`, every subsequent flag may be set false. Conversely a first strict comparison `(0,1)` is forbidden because all preceding equalities forced the flags true. Consequently these weak prefix clauses, existentially quantified, are exactly the required lexicographic comparison. There are 360/296 variables and 1,125/925 clauses.

The high cases use cells `4..11` for the leaf case and `9..11` for the C9 case. They include all twelve-vertex common columns and allocate a flag at every column, including the last. Their clauses enforce exactly `q <-> previous AND (a==b)` (the initial previous value is true): two equality implications, a prefix implication when relevant, and the two equality-to-q clauses just displayed. The guarded comparison clause is also present. Fixed graph units simplify all these clauses. There are 280/30 variables and 1,316/177 resulting clauses.

## Two high blue inputs

Both assert at least 23 edges, minimum degree at least 3, no complementary `K_(4,5)`, the seven-set density condition, and no C8. All graph/counter/lex clauses other than fixed units are simplified by the fixed units. Multiplicities are retained in the positive graph constraints and counter/lex blocks.

- `B-leafK4`: vertices 0,1,2,3 form a K4; vertices 1,2,3 have no neighbors among 4..11. These are 30 units. The C8 clauses are enumerated in the graph of not-fixed-absent edges, simplified by the fixed-present edges, and deduplicated; 22,680 distinct C8 clauses remain.
- `B-C9`: `0,1,...,8,0` is a cycle. Its nine distance-two chords are absent, since any such chord and the complementary seven-edge arc would give C8. These are 18 units. The same C8 procedure leaves 359,226 distinct clauses.

The hand reductions in the main paper cover these high-degree cases. The input program does not invoke an earlier long classification theorem, and it does not assert a new machine-verified coverage proof.

## Meaning of successful comparison

Every original case uses exactly the reconstructed variable numbers: no unverified auxiliary-variable bijection is needed. Every normalized clause, including its multiplicity, must agree. A successful comparison establishes agreement with this explicit implementation. A RUP acceptance additionally establishes unsatisfiability of the supplied formula relative to the correctness of the small checker. The graph interpretation and all case-coverage statements remain readable mathematical arguments as explained here and in the paper.
