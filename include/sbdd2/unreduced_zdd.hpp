// SAPPOROBDD 2.0 - UnreducedZDD class
// MIT License

#ifndef SBDD2_UNREDUCED_ZDD_HPP
#define SBDD2_UNREDUCED_ZDD_HPP

#include "dd_base.hpp"
#include <vector>

namespace sbdd2 {

// Forward declarations
class ZDD;
class QDD;

// UnreducedZDD - Non-reduced Zero-suppressed Decision Diagram
// Allows duplicate nodes and does not apply ZDD reduction rules
// Used for top-down construction algorithms
class UnreducedZDD : public DDBase {
    friend class DDManager;
    friend class ZDD;
    friend class QDD;

public:
    // Default constructor (invalid)
    UnreducedZDD() : DDBase() {}

    // Constructor from manager and arc (internal use)
    UnreducedZDD(DDManager* mgr, Arc a) : DDBase(mgr, a) {}

    // Copy and move
    UnreducedZDD(const UnreducedZDD&) = default;
    UnreducedZDD(UnreducedZDD&&) noexcept = default;
    UnreducedZDD& operator=(const UnreducedZDD&) = default;
    UnreducedZDD& operator=(UnreducedZDD&&) noexcept = default;

    // Static factory methods
    static UnreducedZDD empty(DDManager& mgr);
    static UnreducedZDD base(DDManager& mgr);

    // Create node without reduction (allows redundant nodes)
    static UnreducedZDD node(DDManager& mgr, bddvar var,
                             const UnreducedZDD& low, const UnreducedZDD& high);

    // Low/High children
    UnreducedZDD low() const;
    UnreducedZDD high() const;

    // Convert to reduced ZDD (applies ZDD reduction rules)
    ZDD reduce() const;

    // Convert to QDD (applies node sharing but not reduction)
    QDD to_qdd() const;

    // Check if already reduced
    bool is_reduced() const;
};

} // namespace sbdd2

#endif // SBDD2_UNREDUCED_ZDD_HPP
