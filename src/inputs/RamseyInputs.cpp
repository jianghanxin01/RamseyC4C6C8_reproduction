// RamseyInputs: complete deterministic input reconstruction for nine instances.
// C++14, standard library only, with small platform-specific UTF-8 path helpers.
// This program does NOT solve SAT and does NOT check RUP proofs. It checks the
// graph-to-CNF interpretation, including every auxiliary clause of every case.
#include <algorithm>
#include <cerrno>
#include <chrono>
#include <climits>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <functional>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

namespace ri {
using Clause = std::vector<int>;
using Clauses = std::vector<Clause>;
using Clock = std::chrono::steady_clock;
static void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
static std::string join(const std::string& a, const std::string& b) {
    return a.empty() ? b : a + ((a.back() == '/' || a.back() == '\\') ? "" : "/") + b;
}
#ifdef _WIN32
static std::wstring wide(const std::string& s) {
    if (s.empty()) return std::wstring();
    int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.c_str(), -1, nullptr, 0);
    require(n > 0, "Invalid UTF-8 path");
    std::wstring out(static_cast<size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s.c_str(), -1, &out[0], n);
    out.pop_back(); return out;
}
static std::string utf8(const wchar_t* s) {
    int n = WideCharToMultiByte(CP_UTF8, 0, s, -1, nullptr, 0, nullptr, nullptr);
    require(n > 0, "Cannot encode command-line argument");
    std::string out(static_cast<size_t>(n), '\0');
    WideCharToMultiByte(CP_UTF8, 0, s, -1, &out[0], n, nullptr, nullptr);
    out.pop_back(); return out;
}
#endif
static void openInput(std::ifstream& stream, const std::string& path) {
#ifdef _WIN32
    stream.open(wide(path).c_str(), std::ios::binary);
#else
    stream.open(path.c_str(), std::ios::binary);
#endif
    require(stream.is_open(), "Cannot open input: " + path);
}
static void openOutput(std::ofstream& stream, const std::string& path) {
    // Reproduction must never replace a distributed certificate or a prior
    // report accidentally. Choose a new output path for each generated run.
    std::ifstream existing;
#ifdef _WIN32
    existing.open(wide(path).c_str(), std::ios::binary);
#else
    existing.open(path.c_str(), std::ios::binary);
#endif
    require(!existing.is_open(), "Refusing to overwrite existing file: " + path);
#ifdef _WIN32
    stream.open(wide(path).c_str(), std::ios::binary | std::ios::trunc);
#else
    stream.open(path.c_str(), std::ios::binary | std::ios::trunc);
#endif
    require(stream.is_open(), "Cannot open output: " + path);
}
static void mkdirs(const std::string& path) {
    require(!path.empty(), "Empty output-directory path");
    for (size_t i = 1; i <= path.size(); ++i) {
        if (i != path.size() && path[i] != '/' && path[i] != '\\') continue;
        std::string piece = path.substr(0, i);
        if (piece.empty() || piece.back() == ':' || piece == "/" || piece == "\\") continue;
#ifdef _WIN32
        if (!CreateDirectoryW(wide(piece).c_str(), nullptr)) {
            DWORD e = GetLastError();
            require(e == ERROR_ALREADY_EXISTS, "Cannot create directory: " + piece);
            require((GetFileAttributesW(wide(piece).c_str()) & FILE_ATTRIBUTE_DIRECTORY) != 0,
                    "Output path is not a directory: " + piece);
        }
#else
        if (::mkdir(piece.c_str(), 0777) != 0) {
            require(errno == EEXIST, "Cannot create directory: " + piece);
            struct stat st{};
            require(::stat(piece.c_str(), &st) == 0 && S_ISDIR(st.st_mode),
                    "Output path is not a directory: " + piece);
        }
#endif
    }
}
static std::string jsonString(const std::string& s) {
    std::ostringstream out; out << '"';
    for (unsigned char c : s) {
        if (c == '"') out << "\\\"";
        else if (c == '\\') out << "\\\\";
        else if (c == '\n') out << "\\n";
        else if (c == '\r') out << "\\r";
        else if (c == '\t') out << "\\t";
        else if (c < 32) out << "\\u" << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c) << std::dec;
        else out << static_cast<char>(c);
    }
    out << '"'; return out.str();
}
static Clause canonical(Clause c) {
    std::sort(c.begin(), c.end());
    require(std::adjacent_find(c.begin(), c.end()) == c.end(), "Repeated literal in clause");
    for (int lit : c) {
        require(lit != 0 && lit != INT_MIN, "Invalid literal");
        require(!std::binary_search(c.begin(), c.end(), -lit), "Tautological clause");
    }
    return c;
}
static std::vector<int> range(int begin, int end) {
    std::vector<int> values;
    for (int v = begin; v < end; ++v) values.push_back(v);
    return values;
}
using ChoiceFn = std::function<void(const std::vector<int>&)>;
static void choose(const std::vector<int>& pool, int count, const ChoiceFn& fn) {
    require(count >= 0 && count <= static_cast<int>(pool.size()), "Invalid combination size");
    std::vector<int> selected;
    std::function<void(size_t, int)> rec = [&](size_t from, int left) {
        if (!left) { fn(selected); return; }
        for (size_t i = from; i + static_cast<size_t>(left) <= pool.size(); ++i) {
            selected.push_back(pool[i]); rec(i + 1, left - 1); selected.pop_back();
        }
    };
    rec(0, count);
}
static int edge(int n, int a, int b) {
    if (a > b) std::swap(a, b);
    require(0 <= a && a < b && b < n, "Invalid graph edge");
    return a * (2 * n - a - 1) / 2 + b - a;
}

struct Formula {
    int variables = 0;
    Clauses clauses;
    std::vector<int> fixed; // 0=unfixed, +1=true, -1=false; graph variables only.
    std::map<std::string, size_t> families;
    explicit Formula(int graphVariables) : variables(graphVariables), fixed(static_cast<size_t>(graphVariables + 1), 0) {}
    bool normalize(const Clause& raw, Clause& out, bool simplify) const {
        out.clear();
        for (int lit : raw) {
            int id = std::abs(lit);
            if (simplify && id < static_cast<int>(fixed.size()) && fixed[static_cast<size_t>(id)] != 0) {
                if (fixed[static_cast<size_t>(id)] == (lit > 0 ? 1 : -1)) return false;
            } else out.push_back(lit);
        }
        out = canonical(std::move(out)); return true;
    }
    void add(const Clause& raw, const std::string& family, bool simplify = false) {
        Clause c;
        if (!normalize(raw, c, simplify)) return;
        clauses.push_back(std::move(c)); ++families[family];
    }
    void fix(int id, bool truth) {
        require(id > 0 && id < static_cast<int>(fixed.size()), "Invalid fixed variable");
        int wanted = truth ? 1 : -1;
        require(fixed[static_cast<size_t>(id)] == 0 || fixed[static_cast<size_t>(id)] == wanted, "Conflicting fixed units");
        fixed[static_cast<size_t>(id)] = wanted;
    }
    void units() {
        for (size_t i = 1; i < fixed.size(); ++i)
            if (fixed[i]) add({fixed[i] * static_cast<int>(i)}, "fixed_units");
    }
};

// Fully specified truncated prefix-counter grid, not a call to a SAT library.
// x_i denotes a present edge; S(i,j) means at least j absent edges in x_1..x_i.
// State domain: 1<=j<=k, j<=i<=j+(n-k-1), where k=n-required.
// The numbering is an explicit bijection matching the immutable 53 inputs.
static void atLeast(Formula& f, const std::vector<int>& inputs, int required, bool simplify) {
    const int n = static_cast<int>(inputs.size()), k = n - required;
    require(k > 0 && required >= 2, "Counter parameters outside the audited grid");
    const int start = f.variables;
    auto state = [=](int i, int j) {
        int offset = i - j;
        require(j >= 1 && j <= k && offset >= 0 && offset < required, "Counter state outside grid");
        return offset == 0 ? start + 2 * j - 1 : offset == 1 ? start + 2 * j : start + k * offset + j;
    };
    auto x = [&](int i) { return inputs.at(static_cast<size_t>(i - 1)); };
    for (int i = 1; i <= required; ++i) f.add({x(i), state(i, 1)}, "edge_counter", simplify);
    for (int j = 1; j <= k; ++j)
        for (int i = j; i < j + required - 1; ++i)
            f.add({-state(i, j), state(i + 1, j)}, "edge_counter", simplify);
    for (int j = 2; j <= k; ++j)
        for (int i = j; i < j + required; ++i)
            f.add({x(i), -state(i - 1, j - 1), state(i, j)}, "edge_counter", simplify);
    for (int i = k + 1; i <= n; ++i) f.add({x(i), -state(i - 1, k)}, "edge_counter", simplify);
    f.variables = start + k * required;
}

using Matrix = std::vector<std::vector<int>>;
static Matrix completeIds(int n) {
    Matrix ids(static_cast<size_t>(n), std::vector<int>(static_cast<size_t>(n), 0));
    for (int a = 0; a < n; ++a) for (int b = a + 1; b < n; ++b)
        ids[static_cast<size_t>(a)][static_cast<size_t>(b)] = ids[static_cast<size_t>(b)][static_cast<size_t>(a)] = edge(n, a, b);
    return ids;
}
// A zero entry means that edge cannot occur. Start at the minimum vertex and
// retain exactly one orientation; every simple cycle is emitted exactly once.
static void cycles(const Matrix& ids, int length, int sign, const std::function<void(const Clause&)>& emit) {
    int n = static_cast<int>(ids.size()); std::vector<int> path;
    std::function<void(int, unsigned)> rec = [&](int first, unsigned used) {
        if (static_cast<int>(path.size()) == length) {
            if (ids[static_cast<size_t>(path.back())][static_cast<size_t>(first)] == 0 || path[1] > path.back()) return;
            Clause c;
            for (int i = 0; i < length; ++i)
                c.push_back(sign * ids[static_cast<size_t>(path[static_cast<size_t>(i)])][static_cast<size_t>(path[static_cast<size_t>((i + 1) % length)])]);
            emit(c); return;
        }
        for (int v = first + 1; v < n; ++v) {
            if ((used & (1u << v)) || ids[static_cast<size_t>(path.back())][static_cast<size_t>(v)] == 0) continue;
            path.push_back(v); rec(first, used | (1u << v)); path.pop_back();
        }
    };
    for (int first = 0; first + length <= n; ++first) {
        path.assign(1, first); rec(first, 1u << first);
    }
}

using Emit = std::function<void(const Clause&)>;
static void noComplementK45(const Emit& emit) {
    choose(range(0, 12), 4, [&](const std::vector<int>& A) {
        std::vector<int> outside;
        for (int v = 0; v < 12; ++v) if (std::find(A.begin(), A.end(), v) == A.end()) outside.push_back(v);
        choose(outside, 5, [&](const std::vector<int>& D) {
            Clause c; for (int a : A) for (int b : D) c.push_back(edge(12, a, b)); emit(c);
        });
    });
}
static void sevenDensity(const Emit& emit) {
    choose(range(0, 12), 7, [&](const std::vector<int>& S) {
        Clause all;
        for (size_t i = 0; i < S.size(); ++i) for (size_t j = i + 1; j < S.size(); ++j) all.push_back(edge(12, S[i], S[j]));
        for (size_t omitted = 0; omitted < all.size(); ++omitted) {
            Clause c; for (size_t j = 0; j < all.size(); ++j) if (j != omitted) c.push_back(all[j]); emit(c);
        }
    });
}
static void minimumDegree(int n, int degree, const std::vector<int>& vertices, const Emit& emit) {
    for (int a : vertices) {
        Clause incident;
        for (int b = 0; b < n; ++b) if (a != b) incident.push_back(edge(n, a, b));
        // All present incident edges must fail to fit inside every (d-1)-set.
        choose(incident, degree - 1, [&](const std::vector<int>& allowed) {
            Clause c;
            for (int x : incident) if (std::find(allowed.begin(), allowed.end(), x) == allowed.end()) c.push_back(x);
            emit(c);
        });
    }
}

// Low cases use a weak equality-prefix: equality forces the next flag, but a
// flag may be false after the first strict (1,0) comparison. Under existential
// quantification this is exactly lexicographic >=, not a stronger condition.
static void lowLex(Formula& f, int d) {
    std::vector<std::vector<int>> cells = {range(0, d), range(d, 11)};
    for (const auto& cell : cells) for (size_t i = 0; i < cell.size(); ++i) for (size_t j = i + 1; j < cell.size(); ++j) {
        int u = cell[i], v = cell[j], previous = 0;
        std::vector<int> columns; for (int k = 0; k < 11; ++k) if (k != u && k != v) columns.push_back(k);
        for (size_t index = 0; index < columns.size(); ++index) {
            int a = edge(12, u, columns[index]), b = edge(12, v, columns[index]);
            Clause comparison = {a, -b}; if (previous) comparison.push_back(-previous);
            f.add(comparison, "lex_prefix");
            if (index + 1 == columns.size()) continue;
            int next = ++f.variables;
            Clause bothTrue = {-a, -b, next}, bothFalse = {a, b, next};
            if (previous) { bothTrue.push_back(-previous); bothFalse.push_back(-previous); }
            f.add(bothTrue, "lex_prefix"); f.add(bothFalse, "lex_prefix"); previous = next;
        }
    }
}
// High cases use exact prefixes: next <-> previous AND (a==b).
static void highLex(Formula& f, const std::vector<int>& cell) {
    for (size_t i = 0; i < cell.size(); ++i) for (size_t j = i + 1; j < cell.size(); ++j) {
        int u = cell[i], v = cell[j], previous = 0;
        for (int k = 0; k < 12; ++k) if (k != u && k != v) {
            int a = edge(12, u, k), b = edge(12, v, k), next = ++f.variables;
            Clause comparison = {a, -b}; if (previous) comparison.push_back(-previous);
            f.add(comparison, "lex_prefix", true);
            if (previous) f.add({-next, previous}, "lex_prefix", true);
            f.add({-next, -a, b}, "lex_prefix", true); f.add({-next, a, -b}, "lex_prefix", true);
            Clause bothTrue = {-a, -b, next}, bothFalse = {a, b, next};
            if (previous) { bothTrue.push_back(-previous); bothFalse.push_back(-previous); }
            f.add(bothTrue, "lex_prefix", true); f.add(bothFalse, "lex_prefix", true); previous = next;
        }
    }
}
static void rootSaturation(Formula& f, int d) {
    // For each missing root neighbor, witness a simple length-six path in H.
    // Position 0 chooses a member of P, position 6 is the target, and all
    // internal positions choose among H minus the target. All positions differ.
    for (int target = d; target < 11; ++target) {
        std::vector<std::vector<int>> rows(7, std::vector<int>(11, 0));
        for (int pos = 0; pos < 7; ++pos) {
            Clause row;
            for (int v = 0; v < 11; ++v) {
                bool allowed = pos == 0 ? v < d : pos == 6 ? v == target : v != target;
                if (allowed) { rows[static_cast<size_t>(pos)][static_cast<size_t>(v)] = ++f.variables; row.push_back(f.variables); }
            }
            f.add(row, "root_saturation_paths");
            for (size_t i = 0; i < row.size(); ++i) for (size_t j = i + 1; j < row.size(); ++j)
                f.add({-row[i], -row[j]}, "root_saturation_paths");
        }
        for (int v = 0; v < 11; ++v) {
            Clause uses;
            for (int pos = 0; pos < 7; ++pos) if (rows[static_cast<size_t>(pos)][static_cast<size_t>(v)]) uses.push_back(rows[static_cast<size_t>(pos)][static_cast<size_t>(v)]);
            for (size_t i = 0; i < uses.size(); ++i) for (size_t j = i + 1; j < uses.size(); ++j)
                f.add({-uses[i], -uses[j]}, "root_saturation_paths");
        }
        for (int pos = 0; pos < 6; ++pos) for (int a = 0; a < 11; ++a) for (int b = 0; b < 11; ++b) {
            int av = rows[static_cast<size_t>(pos)][static_cast<size_t>(a)], bv = rows[static_cast<size_t>(pos + 1)][static_cast<size_t>(b)];
            if (av && bv && a != b) f.add({-av, -bv, edge(12, a, b)}, "root_saturation_paths");
        }
    }
}

static Formula blueLow(int d) {
    Formula f(66);
    Matrix h(11, std::vector<int>(11, 0));
    for (int a = 0; a < 11; ++a) for (int b = a + 1; b < 11; ++b) h[static_cast<size_t>(a)][static_cast<size_t>(b)] = h[static_cast<size_t>(b)][static_cast<size_t>(a)] = edge(12, a, b);
    cycles(h, 8, -1, [&](const Clause& c) { f.add(c, "C8_inside_H"); });
    require(f.families["C8_inside_H"] == 415800, "Incomplete H-cycle enumeration");
    if (d == 2) {
        // Every ordered five-tuple from 2..10 completes a six-edge 0--1 path.
        std::vector<int> middle;
        std::function<void(unsigned)> rec = [&](unsigned used) {
            if (middle.size() == 5) {
                Clause c; int previous = 0;
                for (int v : middle) { c.push_back(-edge(12, previous, v)); previous = v; }
                c.push_back(-edge(12, previous, 1)); f.add(c, "C8_through_root"); return;
            }
            for (int v = 2; v < 11; ++v) if (!(used & (1u << v))) {
                middle.push_back(v); rec(used | (1u << v)); middle.pop_back();
            }
        };
        rec(0); require(f.families["C8_through_root"] == 15120, "Incomplete root path enumeration");
    }
    for (int v = 0; v < 11; ++v) f.fix(edge(12, 11, v), v < d);
    std::set<Clause> extra;
    auto insert = [&](const Clause& raw) { Clause c; if (f.normalize(raw, c, true)) extra.insert(std::move(c)); };
    noComplementK45(insert); sevenDensity(insert); minimumDegree(12, d, range(0, 11), insert);
    for (const Clause& c : extra) f.add(c, "positive_graph_constraints_after_root_units");
    f.units();
    std::vector<int> input;
    for (int a = 0; a < 11; ++a) for (int b = a + 1; b < 11; ++b) input.push_back(edge(12, a, b));
    atLeast(f, input, 23 - d, false); lowLex(f, d); rootSaturation(f, d);
    return f;
}
static Formula blueHigh(bool leaf) {
    Formula f(66);
    if (leaf) {
        for (int a = 0; a < 4; ++a) for (int b = a + 1; b < 4; ++b) f.fix(edge(12, a, b), true);
        for (int a = 1; a < 4; ++a) for (int b = 4; b < 12; ++b) f.fix(edge(12, a, b), false);
    } else {
        for (int a = 0; a < 9; ++a) { f.fix(edge(12, a, (a + 1) % 9), true); f.fix(edge(12, a, (a + 2) % 9), false); }
    }
    f.units(); atLeast(f, range(1, 67), 23, true);
    noComplementK45([&](const Clause& c) { f.add(c, "complement_K45", true); });
    sevenDensity([&](const Clause& c) { f.add(c, "seven_set_density", true); });
    minimumDegree(12, 3, range(0, 12), [&](const Clause& c) { f.add(c, "minimum_degree_3", true); });
    highLex(f, leaf ? range(4, 12) : range(9, 12));
    Matrix possible = completeIds(12);
    for (int a = 0; a < 12; ++a) for (int b = a + 1; b < 12; ++b)
        if (f.fixed[static_cast<size_t>(edge(12, a, b))] < 0) possible[static_cast<size_t>(a)][static_cast<size_t>(b)] = possible[static_cast<size_t>(b)][static_cast<size_t>(a)] = 0;
    std::set<Clause> distinct;
    cycles(possible, 8, -1, [&](const Clause& raw) { Clause c; if (f.normalize(raw, c, true)) distinct.insert(std::move(c)); });
    for (const Clause& c : distinct) f.add(c, "C8_after_fixed_units_unique");
    require(distinct.size() == static_cast<size_t>(leaf ? 22680 : 359226), "Incomplete high-case cycle enumeration");
    return f;
}
static Formula hexagon(const std::string& name) {
    Formula f(45); Matrix ids = completeIds(10);
    cycles(ids, 6, -1, [&](const Clause& c) { f.add(c, "no_C6"); });
    choose(range(0, 10), 4, [&](const std::vector<int>& S) {
        Clause c; for (size_t i = 0; i < S.size(); ++i) for (size_t j = i + 1; j < S.size(); ++j) c.push_back(-edge(10, S[i], S[j]));
        f.add(c, "no_K4");
    });
    for (int hub = 0; hub < 10; ++hub) {
        std::vector<int> remaining; for (int v = 0; v < 10; ++v) if (v != hub) remaining.push_back(v);
        choose(remaining, 4, [&](const std::vector<int>& rim) {
            std::vector<int> tail(rim.begin() + 1, rim.end());
            do {
                if (tail.front() > tail.back()) continue;
                std::vector<int> order(1, rim.front()); order.insert(order.end(), tail.begin(), tail.end());
                Clause c; for (int v : rim) c.push_back(-edge(10, hub, v));
                for (int i = 0; i < 4; ++i) c.push_back(-edge(10, order[static_cast<size_t>(i)], order[static_cast<size_t>((i + 1) % 4)]));
                f.add(c, "no_W5");
            } while (std::next_permutation(tail.begin(), tail.end()));
        });
    }
    minimumDegree(10, 3, range(0, 10), [&](const Clause& c) { f.add(c, "minimum_degree_3"); });
    atLeast(f, range(1, 46), 17, false);
    for (int v = 1; v < 10; ++v) f.add({v <= 3 ? edge(10, 0, v) : -edge(10, 0, v)}, "root_units");
    bool first = name != "H10-I3", second = name == "H10-P3";
    f.add({first ? edge(10, 1, 2) : -edge(10, 1, 2)}, "root_units");
    f.add({-edge(10, 1, 3)}, "root_units");
    f.add({second ? edge(10, 2, 3) : -edge(10, 2, 3)}, "root_units");
    require(f.families["no_C6"] == 12600 && f.families["no_K4"] == 210 && f.families["no_W5"] == 3780, "Incomplete H10 obstruction enumeration");
    return f;
}
static Formula arrow(bool first) {
    std::vector<int> group;
    const std::vector<int> sizes = first ? std::vector<int>{4,3,3} : std::vector<int>{4,4,2};
    for (size_t p = 0; p < sizes.size(); ++p) for (int j = 0; j < sizes[p]; ++j) group.push_back(static_cast<int>(p));
    Matrix ids(10, std::vector<int>(10, 0)); int next = 0;
    for (int a = 0; a < 10; ++a) for (int b = a + 1; b < 10; ++b) {
        bool deleted = first ? ((a == 1 && b == 5) || (a == 6 && b == 7)) : (a == 0 && b == 7);
        if (group[static_cast<size_t>(a)] != group[static_cast<size_t>(b)] && !deleted) ids[static_cast<size_t>(a)][static_cast<size_t>(b)] = ids[static_cast<size_t>(b)][static_cast<size_t>(a)] = ++next;
    }
    require(next == 31, "Arrow host does not have 31 edges"); Formula f(31);
    cycles(ids, 4, -1, [&](const Clause& c) { f.add(c, "red_C4"); });
    cycles(ids, 8, 1, [&](const Clause& c) { f.add(c, "blue_C8"); });
    require(f.families["red_C4"] == static_cast<size_t>(first ? 131 : 139) && f.families["blue_C8"] == static_cast<size_t>(first ? 5004 : 4428), "Arrow cycle counts differ");
    return f;
}

struct Case { std::string id, relative; int variables; size_t clauses; };
static const std::vector<Case> cases = {
    {"48T1", "arrows/certificates/48T1.cnf", 31, 5135},
    {"48T2", "arrows/certificates/48T2.cnf", 31, 4567},
    {"H10-I3", "hexagon/hexagon_refined52/I3.cnf", 521, 17903},
    {"H10-K2_K1", "hexagon/hexagon_refined52/K2_K1.cnf", 521, 17903},
    {"H10-P3", "hexagon/hexagon_refined52/P3.cnf", 521, 17903},
    {"B-root-1", "blue/blue_d1_full53.cnf", 1672, 453834},
    {"B-root-2", "blue/blue_d2_full53.cnf", 1553, 462022},
    {"B-leafK4", "blue/blue_high_leafK4/instance.cnf", 1335, 35107},
    {"B-C9", "blue/blue_high_C9/instance.cnf", 1085, 362228}
};
static Formula construct(const Case& spec) {
    Formula f = spec.id == "48T1" ? arrow(true) : spec.id == "48T2" ? arrow(false) :
        spec.id == "B-root-1" ? blueLow(1) : spec.id == "B-root-2" ? blueLow(2) :
        spec.id == "B-leafK4" ? blueHigh(true) : spec.id == "B-C9" ? blueHigh(false) : hexagon(spec.id);
    require(f.variables == spec.variables && f.clauses.size() == spec.clauses,
            "Generated dimensions differ for " + spec.id + ": variables=" + std::to_string(f.variables) + " clauses=" + std::to_string(f.clauses.size()));
    return f;
}

// Strict DIMACS reader. Duplicate/tautological clauses' literals are rejected;
// duplicate whole clauses are retained, since this is a multiset comparison.
// Lines may contain multiple clauses and clauses may span multiple lines.
static Clauses readDimacs(const std::string& path, const Case& expected) {
    std::ifstream input; openInput(input, path);
    bool header = false; Clauses result; Clause pending; std::string line; size_t lineNumber = 0;
    while (std::getline(input, line)) {
        ++lineNumber; size_t first = line.find_first_not_of(" \t\r\n");
        if (first == std::string::npos || line[first] == 'c') continue;
        std::istringstream tokens(line.substr(first));
        if (line[first] == 'p') {
            require(!header && result.empty() && pending.empty(), "Duplicate/misplaced DIMACS header");
            std::string p, cnf, trailing; long long nv = -1, nc = -1;
            require(static_cast<bool>(tokens >> p >> cnf >> nv >> nc) && !(tokens >> trailing), "Malformed DIMACS header");
            require(p == "p" && cnf == "cnf" && nv == expected.variables && nc == static_cast<long long>(expected.clauses),
                    "DIMACS dimensions do not match the declared case: " + path);
            result.reserve(expected.clauses); header = true; continue;
        }
        require(header, "Clause before DIMACS header");
        std::string word;
        while (tokens >> word) {
            char* end = nullptr; errno = 0; long long value = std::strtoll(word.c_str(), &end, 10);
            require(errno != ERANGE && end && *end == '\0' && end != word.c_str(), "Noninteger DIMACS token at line " + std::to_string(lineNumber));
            require(value >= -expected.variables && value <= expected.variables, "Variable outside DIMACS header");
            if (value == 0) {
                require(result.size() < expected.clauses, "Too many DIMACS clauses");
                result.push_back(canonical(std::move(pending))); pending.clear();
            } else pending.push_back(static_cast<int>(value));
        }
    }
    require(input.eof() && header && pending.empty(), "Truncated or unreadable DIMACS file");
    require(result.size() == expected.clauses, "DIMACS clause count mismatch"); return result;
}
static std::string displayClause(const Clause& c) {
    std::ostringstream s; for (int x : c) s << x << ' '; s << '0'; return s.str();
}
static void writeDimacs(const std::string& path, const Formula& f) {
    std::ofstream out; openOutput(out, path); out << "p cnf " << f.variables << ' ' << f.clauses.size() << '\n';
    for (const Clause& c : f.clauses) { for (int x : c) out << x << ' '; out << "0\n"; }
    out.flush(); require(out.good(), "Failed writing generated CNF: " + path);
}
static void help() {
    std::cout << "RamseyInputs (C++14), revision 54\n"
        "Usage: RamseyInputs --all --data ARCHIVE [--generate DIRECTORY] [--report FILE]\n"
        "       RamseyInputs --case ID [--case ID...] --data ARCHIVE [options]\n"
        "       RamseyInputs --all --generate DIRECTORY --generate-only [--report FILE]\n"
        "       RamseyInputs --list\n"
        "Rebuilds every clause, including counters, lex prefixes and root paths.\n"
        "Default: compare sorted clause MULTISETS against immutable archive CNFs.\n"
        "--generate writes separate CNFs. All outputs must be new files.\n"
        "No Python, SAT solver, or external library is required.\n";
}
static int mainImpl(const std::vector<std::string>& args) {
    bool all = false, generateOnly = false; std::vector<std::string> selected;
    std::string data = ".", generate, reportPath; bool dataSpecified = false;
    for (size_t i = 1; i < args.size(); ++i) {
        const std::string& option = args[i];
        if (option == "--help" || option == "-h") { help(); return 0; }
        if (option == "--list") { for (const Case& c : cases) std::cout << c.id << '\n'; return 0; }
        if (option == "--all") { all = true; continue; }
        if (option == "--generate-only") { generateOnly = true; continue; }
        require(option == "--case" || option == "--data" || option == "--generate" || option == "--report", "Unknown option: " + option);
        require(i + 1 < args.size(), "Missing value after " + option); std::string value = args[++i]; require(!value.empty(), "Empty option value");
        if (option == "--case") selected.push_back(value);
        else if (option == "--data") { data = value; dataSpecified = true; }
        else if (option == "--generate") generate = value;
        else reportPath = value;
    }
    require(all != (!selected.empty()), "Choose either --all or one or more --case options");
    require(generateOnly ? !generate.empty() : dataSpecified, "Use --data ARCHIVE, or --generate DIRECTORY --generate-only");
    if (all) for (const Case& c : cases) selected.push_back(c.id);
    std::set<std::string> seen;
    for (const std::string& id : selected) {
        require(seen.insert(id).second, "Repeated case ID: " + id);
        require(std::find_if(cases.begin(), cases.end(), [&](const Case& c) { return c.id == id; }) != cases.end(), "Unknown case ID: " + id);
    }
    if (!generate.empty()) mkdirs(generate);
    auto started = Clock::now(); bool success = true; std::vector<std::string> rows;
    for (const std::string& id : selected) {
        const Case& spec = *std::find_if(cases.begin(), cases.end(), [&](const Case& c) { return c.id == id; });
        auto t = Clock::now(); std::ostringstream row;
        row << "{\"case\":" << jsonString(id) << ",\"variables\":" << spec.variables << ",\"clauses\":" << spec.clauses;
        try {
            Formula f = construct(spec); std::sort(f.clauses.begin(), f.clauses.end());
            if (!generateOnly) {
                Clauses actual = readDimacs(join(data, spec.relative), spec); std::sort(actual.begin(), actual.end());
                auto mismatch = std::mismatch(f.clauses.begin(), f.clauses.end(), actual.begin());
                if (mismatch.first != f.clauses.end()) {
                    throw std::runtime_error("Clause multiset mismatch. Expected [" + displayClause(*mismatch.first) + "]; actual [" + displayClause(*mismatch.second) + "]");
                }
            }
            if (!generate.empty()) {
                const std::string output = join(generate, id + ".cnf");
                // Existing files are never overwritten, including archive inputs.
                writeDimacs(output, f); row << ",\"generated_cnf\":" << jsonString(output);
            }
            row << ",\"status\":" << jsonString(generateOnly ? "GENERATED_NOT_COMPARED" : "PASS_COMPLETE_CLAUSE_MULTISET") << ",\"clause_families\":{";
            bool first = true;
            for (const auto& item : f.families) { if (!first) row << ','; first = false; row << jsonString(item.first) << ':' << item.second; }
            row << '}'; std::cout << id << ": " << (generateOnly ? "GENERATED" : "PASS") << " (" << f.variables << " variables, " << f.clauses.size() << " clauses)\n" << std::flush;
        } catch (const std::exception& e) {
            success = false; row << ",\"status\":\"FAIL\",\"error\":" << jsonString(e.what());
            std::cerr << id << ": FAIL: " << e.what() << '\n';
        }
        row << ",\"seconds\":" << std::fixed << std::setprecision(3) << std::chrono::duration<double>(Clock::now() - t).count() << '}'; rows.push_back(row.str());
    }
    if (!reportPath.empty()) {
        std::ofstream out; openOutput(out, reportPath);
        out << "{\n  \"tool\":\"RamseyInputs C++14 revision54\",\n  \"status\":" << jsonString(success ? "PASS" : "FAIL")
            << ",\n  \"SAT_search_performed\":false,\n  \"proof_check_performed\":false,\n  \"all_auxiliary_blocks_reconstructed\":true,\n  \"mode\":" << jsonString(generateOnly ? "generate-only" : "complete-input-comparison")
            << ",\n  \"cases\":[\n";
        for (size_t i = 0; i < rows.size(); ++i) out << (i ? ",\n" : "") << "    " << rows[i];
        out << "\n  ],\n  \"seconds\":" << std::fixed << std::setprecision(3) << std::chrono::duration<double>(Clock::now() - started).count() << "\n}\n";
        out.flush(); require(out.good(), "Failed writing report");
    }
    std::cout << (success ? (generateOnly ? "ALL REQUESTED CNFS GENERATED (NOT COMPARED)" : "ALL REQUESTED INPUT CHECKS PASSED") : "INPUT CHECK FAILED") << '\n'; return success ? 0 : 1;
}
} // namespace ri

#ifdef _WIN32
int wmain(int argc, wchar_t* argv[]) {
    try {
        SetConsoleOutputCP(CP_UTF8); std::vector<std::string> args;
        for (int i = 0; i < argc; ++i) args.push_back(ri::utf8(argv[i]));
        return ri::mainImpl(args);
    } catch (const std::exception& e) { std::cerr << "ERROR: " << e.what() << '\n'; return 2; }
}
#else
int main(int argc, char* argv[]) {
    try { return ri::mainImpl(std::vector<std::string>(argv, argv + argc)); }
    catch (const std::exception& e) { std::cerr << "ERROR: " << e.what() << '\n'; return 2; }
}
#endif
