// SAPPOROBDD 2.0 - DD Base implementation
// MIT License

#include "sbdd2/dd_base.hpp"
#include <unordered_set>
#include <stack>
#include <algorithm>

namespace sbdd2 {

// Get top variable
bddvar DDBase::top() const {
    if (!manager_ || arc_.is_constant()) {
        return 0;
    }
    return manager_->node_at(arc_.index()).var();
}

// Count nodes in the DD
std::size_t DDBase::size() const {
    if (!manager_) return 0;
    if (arc_.is_constant()) return 0;

    std::unordered_set<bddindex> visited;
    std::stack<Arc> stack;
    stack.push(arc_);

    while (!stack.empty()) {
        Arc current = stack.top();
        stack.pop();

        if (current.is_constant()) continue;

        bddindex idx = current.index();
        if (visited.count(idx)) continue;
        visited.insert(idx);

        const DDNode& node = manager_->node_at(idx);
        stack.push(node.arc0());
        stack.push(node.arc1());
    }

    return visited.size();
}

// Get support (set of variables)
std::vector<bddvar> DDBase::support() const {
    if (!manager_ || arc_.is_constant()) {
        return {};
    }

    std::unordered_set<bddindex> visited_nodes;
    std::unordered_set<bddvar> vars;
    std::stack<Arc> stack;
    stack.push(arc_);

    while (!stack.empty()) {
        Arc current = stack.top();
        stack.pop();

        if (current.is_constant()) continue;

        bddindex idx = current.index();
        if (visited_nodes.count(idx)) continue;
        visited_nodes.insert(idx);

        const DDNode& node = manager_->node_at(idx);
        vars.insert(node.var());
        stack.push(node.arc0());
        stack.push(node.arc1());
    }

    std::vector<bddvar> result(vars.begin(), vars.end());
    std::sort(result.begin(), result.end());
    return result;
}

} // namespace sbdd2
