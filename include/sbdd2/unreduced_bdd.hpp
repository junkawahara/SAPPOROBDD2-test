// SAPPOROBDD 2.0 - UnreducedBDD class
// MIT License

#ifndef SBDD2_UNREDUCED_BDD_HPP
#define SBDD2_UNREDUCED_BDD_HPP

#include "dd_base.hpp"
#include <vector>

namespace sbdd2 {

// Forward declarations
class BDD;
class QDD;

// UnreducedBDD - Non-reduced Binary Decision Diagram
// Allows duplicate nodes and does not apply reduction rules
// Used for top-down construction algorithms
class UnreducedBDD : public DDBase {
    friend class DDManager;
    friend class BDD;
    friend class QDD;

public:
    // Default constructor (invalid)
    UnreducedBDD() : DDBase() {}

    // Constructor from manager and arc (internal use)
    UnreducedBDD(DDManager* mgr, Arc a) : DDBase(mgr, a) {}

    // Copy and move
    UnreducedBDD(const UnreducedBDD&) = default;
    UnreducedBDD(UnreducedBDD&&) noexcept = default;
    UnreducedBDD& operator=(const UnreducedBDD&) = default;
    UnreducedBDD& operator=(UnreducedBDD&&) noexcept = default;

    // Comparison operators are deleted because UnreducedBDD is not canonical
    // (the same logical function can have different node structures)
    bool operator==(const UnreducedBDD&) const = delete;
    bool operator!=(const UnreducedBDD&) const = delete;
    bool operator<(const UnreducedBDD&) const = delete;

    // Static factory methods
    static UnreducedBDD zero(DDManager& mgr);
    static UnreducedBDD one(DDManager& mgr);

    // Create node without reduction (allows redundant nodes)
    static UnreducedBDD node(DDManager& mgr, bddvar var,
                             const UnreducedBDD& low, const UnreducedBDD& high);

    // Low/High children
    UnreducedBDD low() const;
    UnreducedBDD high() const;

    // Negation
    UnreducedBDD operator~() const;

    // Convert to reduced BDD (applies reduction rules)
    BDD reduce() const;

    // Convert to QDD (applies node sharing but not reduction)
    QDD to_qdd() const;

    // Check if already reduced
    bool is_reduced() const;
};

} // namespace sbdd2

#endif // SBDD2_UNREDUCED_BDD_HPP
