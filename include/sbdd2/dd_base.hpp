// SAPPOROBDD 2.0 - DD Base class
// MIT License

#ifndef SBDD2_DD_BASE_HPP
#define SBDD2_DD_BASE_HPP

#include "types.hpp"
#include "dd_manager.hpp"
#include "dd_node_ref.hpp"
#include <string>

namespace sbdd2 {

// DDBase - Common base class for all DD types
class DDBase {
protected:
    DDManager* manager_;
    Arc arc_;

    // Protected constructor
    DDBase() : manager_(nullptr), arc_() {}
    DDBase(DDManager* mgr, Arc a) : manager_(mgr), arc_(a) {
        if (manager_ && !arc_.is_constant()) {
            manager_->inc_ref(arc_);
        }
    }

    // Copy constructor
    DDBase(const DDBase& other)
        : manager_(other.manager_), arc_(other.arc_)
    {
        if (manager_ && !arc_.is_constant()) {
            manager_->inc_ref(arc_);
        }
    }

    // Move constructor
    DDBase(DDBase&& other) noexcept
        : manager_(other.manager_), arc_(other.arc_)
    {
        other.manager_ = nullptr;
        other.arc_ = Arc();
    }

    // Destructor
    ~DDBase() {
        if (manager_ && !arc_.is_constant()) {
            manager_->dec_ref(arc_);
        }
    }

    // Copy assignment
    DDBase& operator=(const DDBase& other) {
        if (this != &other) {
            if (manager_ && !arc_.is_constant()) {
                manager_->dec_ref(arc_);
            }
            manager_ = other.manager_;
            arc_ = other.arc_;
            if (manager_ && !arc_.is_constant()) {
                manager_->inc_ref(arc_);
            }
        }
        return *this;
    }

    // Move assignment
    DDBase& operator=(DDBase&& other) noexcept {
        if (this != &other) {
            if (manager_ && !arc_.is_constant()) {
                manager_->dec_ref(arc_);
            }
            manager_ = other.manager_;
            arc_ = other.arc_;
            other.manager_ = nullptr;
            other.arc_ = Arc();
        }
        return *this;
    }

public:
    // Check if valid
    bool is_valid() const { return manager_ != nullptr; }
    explicit operator bool() const { return is_valid(); }

    // Check if terminal
    bool is_terminal() const { return arc_.is_constant(); }
    bool is_zero() const {
        if (!arc_.is_constant()) return false;
        bool val = arc_.terminal_value() != arc_.is_negated();
        return !val;
    }
    bool is_one() const {
        if (!arc_.is_constant()) return false;
        bool val = arc_.terminal_value() != arc_.is_negated();
        return val;
    }

    // Get top variable (0 for terminal)
    bddvar top() const;

    // Get ID (unique within manager)
    bddindex id() const { return arc_.data; }

    // Get the underlying arc
    Arc arc() const { return arc_; }

    // Get the manager
    DDManager* manager() const { return manager_; }

    // Get DDNodeRef for traversal
    DDNodeRef ref() const {
        return DDNodeRef(manager_, arc_);
    }

    // Size (number of nodes in the DD)
    std::size_t size() const;

    // Support (set of variables)
    std::vector<bddvar> support() const;

    // Comparison
    bool operator==(const DDBase& other) const {
        return manager_ == other.manager_ && arc_ == other.arc_;
    }
    bool operator!=(const DDBase& other) const {
        return !(*this == other);
    }

    // Ordering (for use in containers)
    bool operator<(const DDBase& other) const {
        if (manager_ != other.manager_) {
            return manager_ < other.manager_;
        }
        return arc_.data < other.arc_.data;
    }
};

} // namespace sbdd2

#endif // SBDD2_DD_BASE_HPP
