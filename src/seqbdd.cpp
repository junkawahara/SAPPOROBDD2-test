// SAPPOROBDD 2.0 - SeqBDD implementation
// MIT License

#include "sbdd2/seqbdd.hpp"
#include <iostream>
#include <sstream>
#include <algorithm>

namespace sbdd2 {

// Static factory methods
SeqBDD SeqBDD::empty(DDManager& mgr) {
    return SeqBDD(ZDD::empty(mgr));
}

SeqBDD SeqBDD::base(DDManager& mgr) {
    return SeqBDD(ZDD::base(mgr));
}

SeqBDD SeqBDD::single(DDManager& mgr, bddvar v) {
    return SeqBDD(ZDD::single(mgr, v));
}

// Set operations
SeqBDD SeqBDD::operator&(const SeqBDD& other) const {
    return SeqBDD(zdd_ * other.zdd_);
}

SeqBDD SeqBDD::operator+(const SeqBDD& other) const {
    return SeqBDD(zdd_ + other.zdd_);
}

SeqBDD SeqBDD::operator-(const SeqBDD& other) const {
    return SeqBDD(zdd_ - other.zdd_);
}

// Concatenation
SeqBDD SeqBDD::operator*(const SeqBDD& other) const {
    return SeqBDD(zdd_.product(other.zdd_));
}

// Quotient
SeqBDD SeqBDD::operator/(const SeqBDD& other) const {
    return SeqBDD(zdd_ / other.zdd_);
}

// Remainder
SeqBDD SeqBDD::operator%(const SeqBDD& other) const {
    return *this - other * (*this / other);
}

// Compound assignments
SeqBDD& SeqBDD::operator&=(const SeqBDD& other) { return *this = *this & other; }
SeqBDD& SeqBDD::operator+=(const SeqBDD& other) { return *this = *this + other; }
SeqBDD& SeqBDD::operator-=(const SeqBDD& other) { return *this = *this - other; }
SeqBDD& SeqBDD::operator*=(const SeqBDD& other) { return *this = *this * other; }
SeqBDD& SeqBDD::operator/=(const SeqBDD& other) { return *this = *this / other; }
SeqBDD& SeqBDD::operator%=(const SeqBDD& other) { return *this = *this % other; }

// Sequence operations
SeqBDD SeqBDD::offset(bddvar v) const {
    return SeqBDD(zdd_.offset(v));
}

SeqBDD SeqBDD::onset0(bddvar v) const {
    return SeqBDD(zdd_.onset0(v));
}

SeqBDD SeqBDD::onset(bddvar v) const {
    return onset0(v).push(v);
}

SeqBDD SeqBDD::push(bddvar v) const {
    if (!zdd_.manager()) return SeqBDD();

    // Prepend v to all sequences
    // This creates a new node with v at the top
    DDManager* mgr = zdd_.manager();
    Arc arc = mgr->get_or_create_node_zdd(v, ARC_TERMINAL_0, zdd_.arc(), true);
    return SeqBDD(ZDD(mgr, arc));
}

// Query
bddvar SeqBDD::top() const {
    return zdd_.top();
}

std::size_t SeqBDD::size() const {
    return zdd_.size();
}

double SeqBDD::card() const {
    return zdd_.card();
}

std::size_t SeqBDD::lit() const {
    // Count total literals across all sequences
    auto seqs = enumerate();
    std::size_t total = 0;
    for (const auto& seq : seqs) {
        total += seq.size();
    }
    return total;
}

std::size_t SeqBDD::len() const {
    // Maximum sequence length
    auto seqs = enumerate();
    std::size_t max_len = 0;
    for (const auto& seq : seqs) {
        max_len = std::max(max_len, seq.size());
    }
    return max_len;
}

// Output
std::string SeqBDD::to_string() const {
    std::ostringstream oss;
    oss << "SeqBDD(card=" << card() << ", size=" << size() << ")";
    return oss.str();
}

void SeqBDD::print() const {
    std::cout << to_string() << std::endl;
}

std::vector<std::vector<bddvar>> SeqBDD::enumerate() const {
    return zdd_.enumerate();
}

} // namespace sbdd2
