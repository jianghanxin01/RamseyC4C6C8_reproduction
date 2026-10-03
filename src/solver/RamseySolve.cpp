// Plain-DIMACS Windows wrapper for the official Glucose 4.2.1 core solver.
// Copyright (c) 2026. This wrapper is released under the MIT License.
// See WRAPPER_LICENSE.txt and upstream/LICENSE for the separate notices.
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <fcntl.h>
#include <io.h>
#include <chrono>
#include <climits>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include "core/Solver.h"

struct Formula {
    int variables = 0;
    std::vector<std::vector<int>> clauses;
};

// Decimal tokens are validated before conversion; no prefixes or overflow.
static long long integer(const std::string& token, const std::string& where) {
    if (token.empty()) throw std::runtime_error(where + ": empty integer");
    std::size_t i = token[0] == '-' ? 1 : 0;
    if (i == token.size()) throw std::runtime_error(where + ": sign without digits");
    long long value = 0;
    for (; i < token.size(); ++i) {
        if (token[i] < '0' || token[i] > '9')
            throw std::runtime_error(where + ": invalid integer " + token);
        const int digit = token[i] - '0';
        if (value > (LLONG_MAX - digit) / 10)
            throw std::runtime_error(where + ": integer overflow");
        value = value * 10 + digit;
    }
    if (token[0] == '-' && value == 0)
        throw std::runtime_error(where + ": negative zero is not a terminator");
    return token[0] == '-' ? -value : value;
}

static Formula readFormula(const wchar_t* path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("cannot open input CNF");
    Formula result;
    bool header = false;
    long long expected = -1;
    std::vector<int> clause;
    std::string line;
    std::size_t lineNumber = 0;
    while (std::getline(input, line)) {
        ++lineNumber;
        std::istringstream row(line);
        std::string token;
        if (!(row >> token) || token == "c") continue;
        const std::string where = "line " + std::to_string(lineNumber);
        if (token == "p") {
            if (header) throw std::runtime_error(where + ": duplicate header");
            std::string format, vars, count, extra;
            if (!(row >> format >> vars >> count) || format != "cnf" || (row >> extra))
                throw std::runtime_error(where + ": expected p cnf N M");
            const long long n = integer(vars, where);
            expected = integer(count, where);
            // Glucose encodes each literal in an int as twice its variable.
            if (n < 0 || n > INT_MAX / 2 || expected < 0 || expected > INT_MAX)
                throw std::runtime_error(where + ": unsupported header range");
            result.variables = static_cast<int>(n);
            header = true;
            continue;
        }
        if (!header) throw std::runtime_error(where + ": clause before header");
        do {
            const long long lit = integer(token, where);
            if (lit == 0) {
                if (result.clauses.size() >= static_cast<std::size_t>(expected))
                    throw std::runtime_error(where + ": too many clauses");
                result.clauses.push_back(std::move(clause));
                clause.clear();
            } else {
                if (lit < -result.variables || lit > result.variables)
                    throw std::runtime_error(where + ": literal out of header range");
                clause.push_back(static_cast<int>(lit));
            }
        } while (row >> token);
    }
    if (input.bad()) throw std::runtime_error("input read error");
    if (!header) throw std::runtime_error("missing DIMACS header");
    if (!clause.empty()) throw std::runtime_error("unterminated final clause");
    if (result.clauses.size() != static_cast<std::size_t>(expected))
        throw std::runtime_error("clause count does not match header");
    return result;
}

// CREATE_NEW never truncates an existing proof or an input reached by an alias.
static FILE* newProof(const wchar_t* path) {
    HANDLE handle = CreateFileW(path, GENERIC_WRITE, 0, nullptr, CREATE_NEW,
                                FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handle == INVALID_HANDLE_VALUE)
        throw std::runtime_error("cannot create proof (output must not already exist), Windows error "
                                 + std::to_string(GetLastError()));
    const int descriptor = _open_osfhandle(reinterpret_cast<intptr_t>(handle), _O_WRONLY | _O_BINARY);
    if (descriptor == -1) {
        CloseHandle(handle);
        throw std::runtime_error("cannot attach output descriptor");
    }
    FILE* stream = _fdopen(descriptor, "wb");
    if (!stream) {
        _close(descriptor);
        throw std::runtime_error("cannot attach output stream");
    }
    return stream;
}

int wmain(int argc, wchar_t* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: RamseySolve.exe INPUT.cnf OUTPUT.rup\n"
                  << "Output must be a new file. Exit codes: SAT 10, UNSAT 20, error 2.\n";
        return 2;
    }
    FILE* proof = nullptr;
    try {
        const auto start = std::chrono::steady_clock::now();
        const Formula formula = readFormula(argv[1]);
        const auto parsed = std::chrono::steady_clock::now();
        proof = newProof(argv[2]);
        Glucose::Solver solver;
        solver.verbosity = 0;
        solver.certifiedOutput = proof;
        solver.certifiedUNSAT = true;
        solver.vbyte = false;
        for (int i = 0; i < formula.variables; ++i) solver.newVar();
        // Logging is enabled before even the first input clause is submitted.
        // Retain literal/input order; the core performs its usual normalization.
        for (const auto& raw : formula.clauses) {
            Glucose::vec<Glucose::Lit> clause;
            for (int lit : raw)
                clause.push(Glucose::mkLit((lit < 0 ? -lit : lit) - 1, lit < 0));
            solver.addClause(clause);
        }
        Glucose::vec<Glucose::Lit> assumptions;
        const Glucose::lbool status = solver.solveLimited(assumptions);
        const bool unsat = status == l_False;
        const bool sat = status == l_True;
        if (!unsat && !sat) throw std::runtime_error("solver returned unknown");
        // This covers both root-level loading conflicts and search conflicts.
        // All preceding proof additions are emitted by the upstream core.
        if (unsat && std::fprintf(proof, "0\n") < 0)
            throw std::runtime_error("cannot write final proof clause");
        bool ioError = std::ferror(proof) != 0;
        if (std::fflush(proof) != 0) ioError = true;
        if (std::fclose(proof) != 0) ioError = true;
        proof = nullptr;
        solver.certifiedOutput = nullptr;
        if (ioError) throw std::runtime_error("proof write/close error");
        const auto finished = std::chrono::steady_clock::now();
        std::cout << "s " << (unsat ? "UNSATISFIABLE" : "SATISFIABLE") << '\n';
        if (sat) {
            std::cout << "v";
            for (int i = 0; i < formula.variables; ++i)
                std::cout << ' ' << (solver.modelValue(i) == l_True ? i+1 : -(i+1));
            std::cout << " 0\n";
        }
        std::cout << "c solver=Glucose-4.2.1-core variables=" << formula.variables
                  << " clauses=" << formula.clauses.size()
                  << " conflicts=" << solver.conflicts << std::fixed << std::setprecision(6)
                  << " parse_seconds=" << std::chrono::duration<double>(parsed-start).count()
                  << " solve_seconds=" << std::chrono::duration<double>(finished-parsed).count()
                  << " total_seconds=" << std::chrono::duration<double>(finished-start).count() << '\n';
        return unsat ? 20 : 10;
    } catch (const std::exception& error) {
        if (proof) std::fclose(proof);
        std::cerr << "ERROR: " << error.what() << '\n';
        return 2;
    } catch (...) {
        if (proof) std::fclose(proof);
        std::cerr << "ERROR: solver allocation or unexpected exception\n";
        return 2;
    }
}
