// SAPPOROBDD 2.0 - SeqBDD (Sequence BDD) class
// MIT License

#ifndef SBDD2_SEQBDD_HPP
#define SBDD2_SEQBDD_HPP

#include "zdd.hpp"
#include <vector>
#include <string>

namespace sbdd2 {

// SeqBDD - Sequence Binary Decision Diagram
// Represents sets of sequences using ZDD
class SeqBDD {
private:
    ZDD zdd_;

public:
    // Default constructor (empty set)
    SeqBDD() : zdd_() {}

    // Constructor from ZDD
    explicit SeqBDD(const ZDD& zdd) : zdd_(zdd) {}

    // Copy and move
    SeqBDD(const SeqBDD&) = default;
    SeqBDD(SeqBDD&&) noexcept = default;
    SeqBDD& operator=(const SeqBDD&) = default;
    SeqBDD& operator=(SeqBDD&&) noexcept = default;

    // Static factory methods
    static SeqBDD empty(DDManager& mgr);   // Empty set
    static SeqBDD single(DDManager& mgr);  // Empty sequence only
    static SeqBDD single(DDManager& mgr, bddvar v);  // Single element sequence

    // Set operations
    SeqBDD operator&(const SeqBDD& other) const;  // Intersection
    SeqBDD operator+(const SeqBDD& other) const;  // Union
    SeqBDD operator-(const SeqBDD& other) const;  // Difference
    SeqBDD operator*(const SeqBDD& other) const;  // Concatenation
    SeqBDD operator/(const SeqBDD& other) const;  // Quotient
    SeqBDD operator%(const SeqBDD& other) const;  // Remainder

    // Compound assignments
    SeqBDD& operator&=(const SeqBDD& other);
    SeqBDD& operator+=(const SeqBDD& other);
    SeqBDD& operator-=(const SeqBDD& other);
    SeqBDD& operator*=(const SeqBDD& other);
    SeqBDD& operator/=(const SeqBDD& other);
    SeqBDD& operator%=(const SeqBDD& other);

    // Sequence operations
    SeqBDD offset(bddvar v) const;   // Sequences not starting with v
    SeqBDD onset(bddvar v) const;    // Sequences starting with v
    SeqBDD onset0(bddvar v) const;   // Sequences starting with v (keep v)
    SeqBDD push(bddvar v) const;     // Prepend v to all sequences

    // Query
    bddvar top() const;
    std::size_t size() const;
    double card() const;
    std::size_t lit() const;   // Total number of literals
    std::size_t len() const;   // Maximum sequence length

    // Access underlying ZDD
    const ZDD& get_zdd() const { return zdd_; }
    DDManager* manager() const { return zdd_.manager(); }

    // Comparison
    bool operator==(const SeqBDD& other) const { return zdd_ == other.zdd_; }
    bool operator!=(const SeqBDD& other) const { return zdd_ != other.zdd_; }

    // Output
    std::string to_string() const;
    void print() const;
    std::vector<std::vector<bddvar>> enumerate() const;
};

} // namespace sbdd2

#endif // SBDD2_SEQBDD_HPP
