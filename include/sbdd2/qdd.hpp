// SAPPOROBDD 2.0 - QDD (Quasi-reduced Decision Diagram) class
// MIT License

#ifndef SBDD2_QDD_HPP
#define SBDD2_QDD_HPP

#include "dd_base.hpp"

namespace sbdd2 {

// Forward declarations
class BDD;
class ZDD;
class UnreducedBDD;
class UnreducedZDD;

// QDD - Quasi-reduced Decision Diagram
// Node sharing is enforced (uniqueness guaranteed by hash table)
// But skip rules are NOT applied (every level has a node)
class QDD : public DDBase {
    friend class DDManager;
    friend class BDD;
    friend class ZDD;
    friend class UnreducedBDD;
    friend class UnreducedZDD;

public:
    // Default constructor (invalid)
    QDD() : DDBase() {}

    // Constructor from manager and arc (internal use)
    QDD(DDManager* mgr, Arc a) : DDBase(mgr, a) {}

    // Copy and move
    QDD(const QDD&) = default;
    QDD(QDD&&) noexcept = default;
    QDD& operator=(const QDD&) = default;
    QDD& operator=(QDD&&) noexcept = default;

    // Comparison operators are deleted because QDD is not canonical
    // (skip rules are not applied, so the same function can have different structures)
    bool operator==(const QDD&) const = delete;
    bool operator!=(const QDD&) const = delete;
    bool operator<(const QDD&) const = delete;

    // Create from UnreducedBDD/UnreducedZDD
    explicit QDD(const UnreducedBDD& ubdd);
    explicit QDD(const UnreducedZDD& uzdd);

    // Static factory methods
    static QDD zero(DDManager& mgr);
    static QDD one(DDManager& mgr);

    // Create node with sharing but no skip rule
    static QDD node(DDManager& mgr, bddvar var, const QDD& low, const QDD& high);

    // Low/High children
    QDD low() const;
    QDD high() const;

    // Negation
    QDD operator~() const;

    // Convert to BDD (applies BDD skip rule)
    BDD to_bdd() const;

    // Convert to ZDD (applies ZDD skip rule)
    ZDD to_zdd() const;
};

} // namespace sbdd2

#endif // SBDD2_QDD_HPP
