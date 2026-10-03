// RamseyRup: a portable C++14 reverse-unit-propagation proof checker.
// Prepared for the Ramsey proof archive with OpenAI Codex assistance.
// Adapted from the archive's RupCheck.cs; no SAT-solver code is incorporated.
// No new copyright ownership or licence is asserted by this adaptation.
// See README.md for the accepted formats and the soundness argument.

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

class InvalidProof : public std::runtime_error {
public:
    explicit InvalidProof(const std::string& message) : std::runtime_error(message) {}
};

class RamseyRup {
    struct Clause {
        std::vector<int> literals;
        std::size_t first = 0, second = 1;
        explicit Clause(std::vector<int> data) : literals(std::move(data)) {}
    };
    int variables_ = 0;
    std::vector<Clause> clauses_;
    std::vector<std::vector<std::size_t>> watched_;
    std::vector<int> units_, trail_;
    std::vector<signed char> values_;
    std::size_t head_ = 0;
    bool formulaHasEmpty_ = false;
    std::uint64_t original_ = 0, additions_ = 0, deletions_ = 0;

    static std::string location(const std::string& path, std::uint64_t line) {
        return path + ":" + std::to_string(line);
    }
    // Restrict numeric syntax to ASCII decimal; reject overflow, +1, and -0.
    static std::uint64_t natural(const std::string& token, const std::string& where) {
        if (token.empty()) throw std::runtime_error(where + ": missing integer");
        std::uint64_t value = 0;
        for (char c : token) {
            if (c < '0' || c > '9')
                throw std::runtime_error(where + ": invalid integer token '" + token + "'");
            const unsigned digit = static_cast<unsigned>(c - '0');
            if (value > ((std::numeric_limits<std::uint64_t>::max)() - digit) / 10)
                throw std::runtime_error(where + ": integer overflow");
            value = value * 10 + digit;
        }
        return value;
    }
    int literal(const std::string& token, const std::string& where) const {
        const bool negative = !token.empty() && token[0] == '-';
        const auto magnitude = natural(negative ? token.substr(1) : token, where);
        if ((negative && magnitude == 0) || (magnitude == 0 && token != "0"))
            throw std::runtime_error(where + ": the terminator must be exactly 0");
        if (magnitude > static_cast<std::uint64_t>(variables_))
            throw std::runtime_error(where + ": literal outside the declared variable range");
        const int v = static_cast<int>(magnitude);
        return negative ? -v : v;
    }
    static std::size_t variable(int lit) {
        // INT_MIN is excluded by literal() and the header bound.
        return static_cast<std::size_t>(lit < 0 ? -lit : lit);
    }
    static std::size_t watchIndex(int lit) {
        return 2 * (variable(lit) - 1) + (lit < 0 ? 1 : 0);
    }
    int value(int lit) const {
        const int assigned = values_[variable(lit)];
        return lit > 0 ? assigned : -assigned;
    }
    bool enqueue(int lit) {
        const std::size_t v = variable(lit);
        const signed char sign = static_cast<signed char>(lit > 0 ? 1 : -1);
        if (values_[v] != 0) return values_[v] == sign;
        values_[v] = sign;
        trail_.push_back(lit);
        return true;
    }
    static void normalize(std::vector<int>& clause) {
        // Duplicates are semantically redundant and must not create two
        // watched occurrences of the same literal.
        std::sort(clause.begin(), clause.end());
        clause.erase(std::unique(clause.begin(), clause.end()), clause.end());
    }
    static bool tautology(const std::vector<int>& clause) {
        for (int lit : clause) {
            if (lit >= 0) break;
            if (std::binary_search(clause.begin(), clause.end(), -lit)) return true;
        }
        return false;
    }
    void add(std::vector<int> clause) {
        if (clause.empty()) { formulaHasEmpty_ = true; return; }
        if (tautology(clause)) return;
        if (clause.size() == 1) { units_.push_back(clause[0]); return; }
        const std::size_t id = clauses_.size();
        clauses_.emplace_back(std::move(clause));
        watched_[watchIndex(clauses_.back().literals[0])].push_back(id);
        watched_[watchIndex(clauses_.back().literals[1])].push_back(id);
    }
    bool propagate() {
        while (head_ < trail_.size()) {
            const int falseLiteral = -trail_[head_++];
            auto& list = watched_[watchIndex(falseLiteral)];
            std::size_t position = 0;
            while (position < list.size()) {
                const std::size_t id = list[position];
                Clause& clause = clauses_[id];
                const bool firstIsFalse = clause.literals[clause.first] == falseLiteral;
                if (!firstIsFalse && clause.literals[clause.second] != falseLiteral)
                    throw std::logic_error("internal watched-literal invariant failed");
                const std::size_t otherPosition = firstIsFalse ? clause.second : clause.first;
                const int other = clause.literals[otherPosition];
                if (value(other) == 1) { ++position; continue; }
                std::size_t replacement = clause.literals.size();
                for (std::size_t k = 0; k < clause.literals.size(); ++k) {
                    if (k != clause.first && k != clause.second && value(clause.literals[k]) != -1) {
                        replacement = k;
                        break;
                    }
                }
                if (replacement != clause.literals.size()) {
                    if (firstIsFalse) clause.first = replacement;
                    else clause.second = replacement;
                    list[position] = list.back();
                    list.pop_back();
                    watched_[watchIndex(clause.literals[replacement])].push_back(id);
                } else {
                    if (!enqueue(other)) return false;
                    ++position;
                }
            }
        }
        return true;
    }
    bool isRup(const std::vector<int>& clause) {
        if (formulaHasEmpty_) return true;
        for (int lit : trail_) values_[variable(lit)] = 0;
        trail_.clear();
        head_ = 0;
        for (int lit : units_) if (!enqueue(lit)) return true;
        for (int lit : clause) if (!enqueue(-lit)) return true;
        return !propagate();
    }
    void loadCnf(const std::string& path) {
        std::ifstream file(path);
        if (!file) throw std::runtime_error("cannot open CNF: " + path);
        bool hasHeader = false;
        std::uint64_t declared = 0, lineNumber = 0;
        std::vector<int> pending;
        std::string line;
        while (std::getline(file, line)) {
            ++lineNumber;
            const std::string where = location(path, lineNumber);
            std::istringstream row(line);
            std::string token;
            if (!(row >> token) || token == "c") continue;
            if (token == "p") {
                if (hasHeader) throw std::runtime_error(where + ": duplicate CNF header");
                std::string format, vars, count, extra;
                if (!(row >> format >> vars >> count) || format != "cnf" || (row >> extra))
                    throw std::runtime_error(where + ": expected exactly p cnf VARIABLES CLAUSES");
                const auto n = natural(vars, where);
                if (n > static_cast<std::uint64_t>((std::numeric_limits<int>::max)()) ||
                    n > (std::numeric_limits<std::size_t>::max)() / 2)
                    throw std::runtime_error(where + ": variable count too large for this build");
                variables_ = static_cast<int>(n);
                declared = natural(count, where);
                values_.assign(static_cast<std::size_t>(variables_) + 1, 0);
                watched_.resize(2 * static_cast<std::size_t>(variables_));
                hasHeader = true;
                continue;
            }
            if (!hasHeader) throw std::runtime_error(where + ": clause before CNF header");
            do {
                const int lit = literal(token, where);
                if (lit == 0) {
                    if (original_ == declared)
                        throw std::runtime_error(where + ": more clauses than declared");
                    normalize(pending);
                    add(std::move(pending));
                    pending.clear();
                    ++original_;
                } else pending.push_back(lit);
            } while (row >> token);
        }
        if (file.bad()) throw std::runtime_error("CNF read error: " + path);
        if (!hasHeader) throw std::runtime_error("missing CNF header: " + path);
        if (!pending.empty()) throw std::runtime_error("unterminated CNF clause: " + path);
        if (original_ != declared) throw std::runtime_error("CNF clause count mismatch: " + path);
    }
    void checkProof(const std::string& path) {
        std::ifstream file(path);
        if (!file) throw std::runtime_error("cannot open proof: " + path);
        bool finalAdditionEmpty = false;
        std::uint64_t lineNumber = 0;
        std::string line;
        while (std::getline(file, line)) {
            ++lineNumber;
            const std::string where = location(path, lineNumber);
            std::istringstream row(line);
            std::string token;
            if (!(row >> token) || token == "c") continue;
            const bool deletion = token == "d";
            if (deletion && !(row >> token))
                throw std::runtime_error(where + ": missing deletion clause");
            std::vector<int> clause;
            bool terminated = false;
            do {
                const int lit = literal(token, where);
                if (lit == 0) {
                    terminated = true;
                    if (row >> token) throw std::runtime_error(where + ": extra token after proof terminator");
                    break;
                }
                clause.push_back(lit);
            } while (row >> token);
            if (!terminated) throw std::runtime_error(where + ": unterminated proof clause");
            if (deletion) { ++deletions_; continue; }
            normalize(clause);
            if (!isRup(clause))
                throw InvalidProof(where + ": invalid RUP addition " + std::to_string(additions_ + 1));
            finalAdditionEmpty = clause.empty();
            add(std::move(clause));
            ++additions_;
        }
        if (file.bad()) throw std::runtime_error("proof read error: " + path);
        if (!finalAdditionEmpty)
            throw InvalidProof("proof must end with an explicit empty-clause addition");
    }
public:
    void run(const std::string& cnf, const std::string& proof) {
        const auto start = std::chrono::steady_clock::now();
        loadCnf(cnf);
        const auto loaded = std::chrono::steady_clock::now();
        checkProof(proof);
        const auto finished = std::chrono::steady_clock::now();
        std::cout << "VERIFIED variables=" << variables_ << " original=" << original_
                  << " additions=" << additions_ << " deletions_ignored=" << deletions_
                  << " final_empty=1" << std::fixed << std::setprecision(6)
                  << " load_seconds=" << std::chrono::duration<double>(loaded - start).count()
                  << " proof_seconds=" << std::chrono::duration<double>(finished - loaded).count()
                  << " total_seconds=" << std::chrono::duration<double>(finished - start).count() << '\n';
    }
};

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cerr << "Usage: RamseyRup.exe INPUT.cnf PROOF.rup\n";
        return 2;
    }
    try {
        RamseyRup checker;
        checker.run(argv[1], argv[2]);
        return 0;
    } catch (const InvalidProof& error) {
        std::cerr << "REJECTED: " << error.what() << '\n';
        return 1;
    } catch (const std::exception& error) {
        std::cerr << "ERROR: " << error.what() << '\n';
        return 2;
    }
}
