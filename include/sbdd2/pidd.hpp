// SAPPOROBDD 2.0 - PiDD (Permutation DD) class
// MIT License

#ifndef SBDD2_PIDD_HPP
#define SBDD2_PIDD_HPP

#include "zdd.hpp"
#include <vector>

namespace sbdd2 {

// PiDD - Permutation Decision Diagram
// Represents sets of permutations using ZDD
class PiDD {
public:
    static constexpr int MAX_VAR = 254;

private:
    ZDD zdd_;

    // Variable mapping tables
    static std::vector<int> lev_of_x_;  // Level of variable x
    static std::vector<int> x_of_lev_;  // Variable at level
    static int top_var_;
    static DDManager* manager_;

public:
    // Default constructor (empty set)
    PiDD() : zdd_() {}

    // Constructor from ZDD
    explicit PiDD(const ZDD& zdd) : zdd_(zdd) {}

    // Copy and move
    PiDD(const PiDD&) = default;
    PiDD(PiDD&&) noexcept = default;
    PiDD& operator=(const PiDD&) = default;
    PiDD& operator=(PiDD&&) noexcept = default;

    // Initialize PiDD system
    static void init(DDManager& mgr);

    // Create new variable
    static int new_var();

    // Get number of variables used
    static int var_used();

    // Static factory methods
    static PiDD empty();   // Empty set
    static PiDD base();    // Identity permutation only
    static PiDD single(int x, int y);  // Single transposition (x,y)

    // Set operations
    PiDD operator&(const PiDD& other) const;  // Intersection
    PiDD operator+(const PiDD& other) const;  // Union
    PiDD operator-(const PiDD& other) const;  // Difference
    PiDD operator*(const PiDD& other) const;  // Composition
    PiDD operator/(const PiDD& other) const;  // Quotient
    PiDD operator%(const PiDD& other) const;  // Remainder

    // Compound assignments
    PiDD& operator&=(const PiDD& other);
    PiDD& operator+=(const PiDD& other);
    PiDD& operator-=(const PiDD& other);
    PiDD& operator*=(const PiDD& other);
    PiDD& operator/=(const PiDD& other);
    PiDD& operator%=(const PiDD& other);

    // Permutation operations
    PiDD swap(int x, int y) const;   // Swap elements x and y
    PiDD cofact(int x, int y) const; // Cofactor at (x,y)
    PiDD odd() const;                // Odd permutations only
    PiDD even() const;               // Even permutations only
    PiDD swap_bound(int n) const;    // Permutations bounded by n

    // Query
    int top_x() const;
    int top_y() const;
    int top_lev() const;
    std::size_t size() const;
    double card() const;

    // Access underlying ZDD
    const ZDD& get_zdd() const { return zdd_; }

    // Comparison
    bool operator==(const PiDD& other) const { return zdd_ == other.zdd_; }
    bool operator!=(const PiDD& other) const { return zdd_ != other.zdd_; }

    // Output
    std::string to_string() const;
    void print() const;
    std::vector<std::vector<int>> enumerate() const;

private:
    // Helper functions
    static int x_of_level(int lev);
    static int y_of_level(int lev);
    static int level_of_xy(int x, int y);
};

} // namespace sbdd2

#endif // SBDD2_PIDD_HPP
