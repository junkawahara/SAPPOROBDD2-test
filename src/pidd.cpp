// SAPPOROBDD 2.0 - PiDD implementation
// MIT License

#include "sbdd2/pidd.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>

namespace sbdd2 {

// Static members
std::vector<int> PiDD::lev_of_x_;
std::vector<int> PiDD::x_of_lev_;
int PiDD::top_var_ = 0;
DDManager* PiDD::manager_ = nullptr;

// Initialize PiDD system
void PiDD::init(DDManager& mgr) {
    manager_ = &mgr;
    top_var_ = 0;
    lev_of_x_.clear();
    x_of_lev_.clear();
    lev_of_x_.resize(MAX_VAR + 1, 0);
    x_of_lev_.resize(MAX_VAR * MAX_VAR + 1, 0);
}

// Create new variable
int PiDD::new_var() {
    if (!manager_) return -1;
    if (top_var_ >= MAX_VAR) return -1;

    top_var_++;
    int x = top_var_;

    // Set up level mapping for new variable
    // Each variable x creates levels for pairs (x, y) where y < x
    int base_level = lev_of_x_[x - 1];
    lev_of_x_[x] = base_level + x - 1;

    for (int y = 1; y < x; y++) {
        int lev = base_level + y;
        x_of_lev_[lev] = x;
        manager_->new_var();  // Create corresponding BDD variable
    }

    return x;
}

// Get number of variables used
int PiDD::var_used() {
    return top_var_;
}

// Helper functions
int PiDD::x_of_level(int lev) {
    return x_of_lev_[lev];
}

int PiDD::y_of_level(int lev) {
    return lev_of_x_[x_of_lev_[lev]] - lev + 1;
}

int PiDD::level_of_xy(int x, int y) {
    return lev_of_x_[x] - y + 1;
}

// Static factory methods
PiDD PiDD::empty() {
    if (!manager_) return PiDD();
    return PiDD(ZDD::empty(*manager_));
}

PiDD PiDD::base() {
    if (!manager_) return PiDD();
    return PiDD(ZDD::base(*manager_));
}

PiDD PiDD::single(int x, int y) {
    if (!manager_ || x <= 0 || y <= 0 || x == y) return PiDD();
    if (x < y) std::swap(x, y);

    int lev = level_of_xy(x, y);
    bddvar var = static_cast<bddvar>(lev);
    return PiDD(ZDD::single(*manager_, var));
}

// Set operations
PiDD PiDD::operator&(const PiDD& other) const {
    return PiDD(zdd_ * other.zdd_);  // Intersection
}

PiDD PiDD::operator+(const PiDD& other) const {
    return PiDD(zdd_ + other.zdd_);  // Union
}

PiDD PiDD::operator-(const PiDD& other) const {
    return PiDD(zdd_ - other.zdd_);  // Difference
}

// Composition of permutations
PiDD PiDD::operator*(const PiDD& other) const {
    // Permutation composition is complex
    // This is a simplified version
    return PiDD(zdd_.product(other.zdd_));
}

// Quotient
PiDD PiDD::operator/(const PiDD& other) const {
    return PiDD(zdd_ / other.zdd_);
}

// Remainder
PiDD PiDD::operator%(const PiDD& other) const {
    return *this - (*this / other) * other;
}

// Compound assignments
PiDD& PiDD::operator&=(const PiDD& other) { return *this = *this & other; }
PiDD& PiDD::operator+=(const PiDD& other) { return *this = *this + other; }
PiDD& PiDD::operator-=(const PiDD& other) { return *this = *this - other; }
PiDD& PiDD::operator*=(const PiDD& other) { return *this = *this * other; }
PiDD& PiDD::operator/=(const PiDD& other) { return *this = *this / other; }
PiDD& PiDD::operator%=(const PiDD& other) { return *this = *this % other; }

// Permutation operations
PiDD PiDD::swap(int x, int y) const {
    if (x <= 0 || y <= 0 || x == y) return *this;
    if (x < y) std::swap(x, y);

    PiDD swap_perm = single(x, y);
    return *this * swap_perm;
}

PiDD PiDD::cofact(int x, int y) const {
    if (x <= 0 || y <= 0 || x == y) return *this;
    if (x < y) std::swap(x, y);

    int lev = level_of_xy(x, y);
    bddvar var = static_cast<bddvar>(lev);
    return PiDD(zdd_.onset(var));
}

PiDD PiDD::odd() const {
    // Returns permutations with odd number of transpositions
    // This requires computing parity
    PiDD result = empty();
    auto perms = enumerate();
    for (const auto& perm : perms) {
        if (perm.size() % 2 == 1) {
            // Reconstruct PiDD for this permutation
            PiDD p = base();
            for (size_t i = 0; i + 1 < perm.size(); i += 2) {
                p = p * single(perm[i], perm[i + 1]);
            }
            result = result + p;
        }
    }
    return result;
}

PiDD PiDD::even() const {
    return *this - odd();
}

PiDD PiDD::swap_bound(int n) const {
    // Restrict to permutations of elements <= n
    PiDD result = *this;
    for (int x = n + 1; x <= top_var_; x++) {
        for (int y = 1; y < x; y++) {
            result = result - result.cofact(x, y);
        }
    }
    return result;
}

// Query
int PiDD::top_x() const {
    return x_of_level(top_lev());
}

int PiDD::top_y() const {
    return y_of_level(top_lev());
}

int PiDD::top_lev() const {
    return static_cast<int>(zdd_.top());
}

std::size_t PiDD::size() const {
    return zdd_.size();
}

double PiDD::card() const {
    return zdd_.card();
}

// Output
std::string PiDD::to_string() const {
    std::ostringstream oss;
    oss << "PiDD(card=" << card() << ", size=" << size() << ")";
    return oss.str();
}

void PiDD::print() const {
    std::cout << to_string() << std::endl;
}

std::vector<std::vector<int>> PiDD::enumerate() const {
    auto sets = zdd_.enumerate();
    std::vector<std::vector<int>> result;

    for (const auto& s : sets) {
        std::vector<int> perm;
        for (bddvar v : s) {
            int lev = static_cast<int>(v);
            int x = x_of_level(lev);
            int y = y_of_level(lev);
            perm.push_back(x);
            perm.push_back(y);
        }
        result.push_back(perm);
    }

    return result;
}

} // namespace sbdd2
