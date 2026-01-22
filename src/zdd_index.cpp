// SAPPOROBDD 2.0 - ZDD Index implementation
// MIT License

#include "sbdd2/zdd.hpp"
#include <queue>
#include <algorithm>

#ifdef SBDD2_HAS_GMP
#include <gmpxx.h>
#endif

namespace sbdd2 {

// Internal helper to get 0-child arc for ZDD
static Arc get_child0_zdd(DDManager* mgr, Arc arc) {
    if (arc.is_constant()) {
        return arc;
    }
    const DDNode& node = mgr->node_at(arc.index());
    return node.arc0();
}

// Internal helper to get 1-child arc for ZDD
static Arc get_child1_zdd(DDManager* mgr, Arc arc) {
    if (arc.is_constant()) {
        return arc;
    }
    const DDNode& node = mgr->node_at(arc.index());
    return node.arc1();
}

// Internal helper to get level of arc
static int get_level(DDManager* mgr, Arc arc) {
    if (arc.is_constant()) {
        return 0;
    }
    const DDNode& node = mgr->node_at(arc.index());
    return static_cast<int>(node.var());
}

void ZDD::build_index() const {
    if (!manager_ || !index_once_flag_) {
        return;
    }
    std::call_once(*index_once_flag_, [this]() {
        build_index_impl();
    });
}

void ZDD::ensure_index() const {
    build_index();
}

void ZDD::build_index_impl() const {
    // Create index data structure
    index_cache_ = std::unique_ptr<ZDDIndexData>(new ZDDIndexData());

    // Handle terminal cases
    if (is_zero()) {
        index_cache_->height = 0;
        return;
    }
    if (is_one()) {
        index_cache_->height = 0;
        // Base (1-terminal) has count 1, which we handle specially
        return;
    }

    // In SAPPOROBDD2, level numbers are:
    // - Level 1 is the "top" (closest to root)
    // - Higher level numbers are closer to terminals
    // - Children have HIGHER level numbers than parents

    // First pass: BFS to find max level and collect all nodes
    Arc root = arc_;
    if (root.is_negated()) {
        root = Arc::node(root.index(), false);
    }

    int root_level = get_level(manager_, root);
    int max_level = root_level;

    // Temporary storage for BFS
    std::vector<Arc> all_nodes;
    std::unordered_map<Arc, bool, ArcHash, ArcEqual> visited;

    std::queue<Arc> bfs_queue;
    bfs_queue.push(root);
    visited[root] = true;
    all_nodes.push_back(root);

    while (!bfs_queue.empty()) {
        Arc node = bfs_queue.front();
        bfs_queue.pop();

        Arc child0 = get_child0_zdd(manager_, node);
        Arc child1 = get_child1_zdd(manager_, node);

        if (!child0.is_constant() && visited.find(child0) == visited.end()) {
            visited[child0] = true;
            all_nodes.push_back(child0);
            int child_level = get_level(manager_, child0);
            if (child_level > max_level) max_level = child_level;
            bfs_queue.push(child0);
        }

        if (!child1.is_constant() && visited.find(child1) == visited.end()) {
            visited[child1] = true;
            all_nodes.push_back(child1);
            int child_level = get_level(manager_, child1);
            if (child_level > max_level) max_level = child_level;
            bfs_queue.push(child1);
        }
    }

    index_cache_->height = max_level;

    // Initialize level_nodes array (level 0 unused, level 1 to max_level used)
    index_cache_->level_nodes.resize(max_level + 1);

    // Organize nodes by level
    for (const Arc& node : all_nodes) {
        int lev = get_level(manager_, node);
        index_cache_->node_to_idx[node] = index_cache_->level_nodes[lev].size();
        index_cache_->level_nodes[lev].push_back(node);
    }

    // Compute counts bottom-up
    // count[0-terminal] = 0, count[1-terminal] = 1
    // For each node: count[node] = count[0-child] + count[1-child]

    auto get_count = [this](Arc a) -> double {
        if (a.is_constant()) {
            // 0-terminal (empty) -> 0, 1-terminal (base) -> 1
            return (a == ARC_TERMINAL_1) ? 1.0 : 0.0;
        }
        auto it = index_cache_->count_cache.find(a);
        if (it != index_cache_->count_cache.end()) {
            return it->second;
        }
        return 0.0;  // Should not happen if processing order is correct
    };

    // Process from bottom to top (high level number to low level number)
    // Level max_level is closest to terminals, level root_level is at root
    for (int lev = max_level; lev >= root_level; --lev) {
        for (std::size_t j = 0; j < index_cache_->level_nodes[lev].size(); ++j) {
            Arc node = index_cache_->level_nodes[lev][j];
            Arc child0 = get_child0_zdd(manager_, node);
            Arc child1 = get_child1_zdd(manager_, node);

            double count0 = get_count(child0);
            double count1 = get_count(child1);

            index_cache_->count_cache[node] = count0 + count1;
        }
    }
}

#ifdef SBDD2_HAS_GMP
void ZDD::build_exact_index() const {
    if (!manager_ || !exact_index_once_flag_) {
        return;
    }
    std::call_once(*exact_index_once_flag_, [this]() {
        build_exact_index_impl();
    });
}

void ZDD::ensure_exact_index() const {
    build_exact_index();
}

void ZDD::build_exact_index_impl() const {
    // Create index data structure
    exact_index_cache_ = std::unique_ptr<ZDDExactIndexData>(new ZDDExactIndexData());

    // Handle terminal cases
    if (is_zero()) {
        exact_index_cache_->height = 0;
        return;
    }
    if (is_one()) {
        exact_index_cache_->height = 0;
        return;
    }

    // In SAPPOROBDD2, level numbers are:
    // - Level 1 is the "top" (closest to root)
    // - Higher level numbers are closer to terminals
    // - Children have HIGHER level numbers than parents

    Arc root = arc_;
    if (root.is_negated()) {
        root = Arc::node(root.index(), false);
    }

    int root_level = get_level(manager_, root);
    int max_level = root_level;

    // BFS to find all nodes and max level
    std::vector<Arc> all_nodes;
    std::unordered_map<Arc, bool, ArcHash, ArcEqual> visited;

    std::queue<Arc> bfs_queue;
    bfs_queue.push(root);
    visited[root] = true;
    all_nodes.push_back(root);

    while (!bfs_queue.empty()) {
        Arc node = bfs_queue.front();
        bfs_queue.pop();

        Arc child0 = get_child0_zdd(manager_, node);
        Arc child1 = get_child1_zdd(manager_, node);

        if (!child0.is_constant() && visited.find(child0) == visited.end()) {
            visited[child0] = true;
            all_nodes.push_back(child0);
            int child_level = get_level(manager_, child0);
            if (child_level > max_level) max_level = child_level;
            bfs_queue.push(child0);
        }

        if (!child1.is_constant() && visited.find(child1) == visited.end()) {
            visited[child1] = true;
            all_nodes.push_back(child1);
            int child_level = get_level(manager_, child1);
            if (child_level > max_level) max_level = child_level;
            bfs_queue.push(child1);
        }
    }

    exact_index_cache_->height = max_level;
    exact_index_cache_->level_nodes.resize(max_level + 1);

    // Organize nodes by level
    for (const Arc& node : all_nodes) {
        int lev = get_level(manager_, node);
        exact_index_cache_->node_to_idx[node] = exact_index_cache_->level_nodes[lev].size();
        exact_index_cache_->level_nodes[lev].push_back(node);
    }

    // Compute counts bottom-up with GMP
    auto get_count = [this](Arc a) -> mpz_class {
        if (a.is_constant()) {
            return (a == ARC_TERMINAL_1) ? mpz_class(1) : mpz_class(0);
        }
        auto it = exact_index_cache_->count_cache.find(a);
        if (it != exact_index_cache_->count_cache.end()) {
            return it->second;
        }
        return mpz_class(0);
    };

    // Process from bottom to top (high level number to low level number)
    for (int lev = max_level; lev >= root_level; --lev) {
        for (std::size_t j = 0; j < exact_index_cache_->level_nodes[lev].size(); ++j) {
            Arc node = exact_index_cache_->level_nodes[lev][j];
            Arc child0 = get_child0_zdd(manager_, node);
            Arc child1 = get_child1_zdd(manager_, node);

            mpz_class count0 = get_count(child0);
            mpz_class count1 = get_count(child1);

            exact_index_cache_->count_cache[node] = count0 + count1;
        }
    }
}
#endif

void ZDD::clear_index() const {
    // Reset the once_flags and clear caches
    // Note: This is not thread-safe with concurrent build operations,
    // but the design assumes clear_index is called when no other threads
    // are accessing the index.
    index_once_flag_.reset(new std::once_flag());
    index_cache_.reset();

#ifdef SBDD2_HAS_GMP
    exact_index_once_flag_.reset(new std::once_flag());
    exact_index_cache_.reset();
#endif
}

bool ZDD::has_index() const {
    return index_cache_ != nullptr;
}

#ifdef SBDD2_HAS_GMP
bool ZDD::has_exact_index() const {
    return exact_index_cache_ != nullptr;
}
#endif

int ZDD::index_height() const {
    ensure_index();
    if (!index_cache_) {
        return 0;
    }
    return index_cache_->height;
}

std::size_t ZDD::index_size() const {
    ensure_index();
    if (!index_cache_) {
        return 0;
    }

    std::size_t total = 0;
    for (int lev = 1; lev <= index_cache_->height; ++lev) {
        total += index_cache_->level_nodes[lev].size();
    }
    return total;
}

std::size_t ZDD::index_size_at_level(int level) const {
    ensure_index();
    if (!index_cache_ || level <= 0 || level > index_cache_->height) {
        return 0;
    }
    return index_cache_->level_nodes[level].size();
}

double ZDD::indexed_count() const {
    // Handle terminal cases directly
    if (is_zero()) {
        return 0.0;
    }
    if (is_one()) {
        return 1.0;
    }

    ensure_index();
    if (!index_cache_ || index_cache_->height == 0) {
        return 0.0;
    }

    // Get count of root node
    Arc root = arc_;
    if (root.is_negated()) {
        root = Arc::node(root.index(), false);
    }

    auto it = index_cache_->count_cache.find(root);
    if (it != index_cache_->count_cache.end()) {
        return it->second;
    }

    return 0.0;
}

#ifdef SBDD2_HAS_GMP
std::string ZDD::indexed_exact_count() const {
    // Handle terminal cases directly
    if (is_zero()) {
        return "0";
    }
    if (is_one()) {
        return "1";
    }

    ensure_exact_index();
    if (!exact_index_cache_ || exact_index_cache_->height == 0) {
        return "0";
    }

    Arc root = arc_;
    if (root.is_negated()) {
        root = Arc::node(root.index(), false);
    }

    auto it = exact_index_cache_->count_cache.find(root);
    if (it != exact_index_cache_->count_cache.end()) {
        return it->second.get_str();
    }

    return "0";
}
#endif

// ============== Dictionary Order Methods ==============

// Helper: Check if empty set is a member of the family rooted at arc
static bool is_empty_member(DDManager* mgr, Arc arc) {
    // Empty set is a member if we can reach 1-terminal by taking only 0-children
    while (!arc.is_constant()) {
        const DDNode& node = mgr->node_at(arc.index());
        arc = node.arc0();  // Follow 0-child
    }
    return arc == ARC_TERMINAL_1;
}

// Helper: Get count stored for an arc
static double get_arc_count(const ZDDIndexData* cache, Arc arc) {
    if (arc.is_constant()) {
        return (arc == ARC_TERMINAL_1) ? 1.0 : 0.0;
    }
    auto it = cache->count_cache.find(arc);
    if (it != cache->count_cache.end()) {
        return it->second;
    }
    return 0.0;
}

int64_t ZDD::order_of(const std::set<bddvar>& s) const {
    // Handle terminal cases
    if (is_zero()) {
        return -1;  // Empty family contains no sets
    }
    if (is_one()) {
        // Base family {∅} contains only empty set
        return s.empty() ? 0 : -1;
    }

    ensure_index();
    if (!index_cache_) {
        return -1;
    }

    // Make a mutable copy of the input set
    std::set<bddvar> remaining = s;

    // Start from root
    Arc current = arc_;
    if (current.is_negated()) {
        current = Arc::node(current.index(), false);
    }

    int64_t order = 0;

    while (!current.is_constant()) {
        const DDNode& node = manager_->node_at(current.index());
        bddvar var = node.var();

        Arc child0 = node.arc0();
        Arc child1 = node.arc1();

        if (remaining.count(var) > 0) {
            // Variable is in the set, follow 1-child
            remaining.erase(var);
            current = child1;
        } else {
            // Variable is not in the set, follow 0-child
            // But first, add the count of the 1-child subtree to order
            double count1 = get_arc_count(index_cache_.get(), child1);
            order += static_cast<int64_t>(count1);
            current = child0;
        }
    }

    // At terminal: check if we found the set
    if (current == ARC_TERMINAL_1 && remaining.empty()) {
        return order;
    }

    return -1;  // Set not found
}

std::set<bddvar> ZDD::get_set(int64_t order) const {
    std::set<bddvar> result;

    // Handle terminal cases
    if (is_zero()) {
        return result;  // Empty family, return empty set
    }
    if (is_one()) {
        // Base family {∅}, order 0 gives empty set
        return result;
    }

    if (order < 0) {
        return result;
    }

    ensure_index();
    if (!index_cache_) {
        return result;
    }

    // Start from root
    Arc current = arc_;
    if (current.is_negated()) {
        current = Arc::node(current.index(), false);
    }

    while (!current.is_constant()) {
        const DDNode& node = manager_->node_at(current.index());
        bddvar var = node.var();

        Arc child1 = node.arc1();
        Arc child0 = node.arc0();

        double count1 = get_arc_count(index_cache_.get(), child1);
        int64_t count1_int = static_cast<int64_t>(count1);

        if (order < count1_int) {
            // The set is in the 1-child subtree (contains this variable)
            result.insert(var);
            current = child1;
        } else {
            // The set is in the 0-child subtree (doesn't contain this variable)
            order -= count1_int;
            current = child0;
        }
    }

    return result;
}

#ifdef SBDD2_HAS_GMP
// Helper: Get GMP count stored for an arc
static mpz_class get_arc_count_gmp(const ZDDExactIndexData* cache, Arc arc) {
    if (arc.is_constant()) {
        return (arc == ARC_TERMINAL_1) ? mpz_class(1) : mpz_class(0);
    }
    auto it = cache->count_cache.find(arc);
    if (it != cache->count_cache.end()) {
        return it->second;
    }
    return mpz_class(0);
}

std::string ZDD::exact_order_of(const std::set<bddvar>& s) const {
    // Handle terminal cases
    if (is_zero()) {
        return "-1";
    }
    if (is_one()) {
        return s.empty() ? "0" : "-1";
    }

    ensure_exact_index();
    if (!exact_index_cache_) {
        return "-1";
    }

    std::set<bddvar> remaining = s;
    Arc current = arc_;
    if (current.is_negated()) {
        current = Arc::node(current.index(), false);
    }

    mpz_class order(0);

    while (!current.is_constant()) {
        const DDNode& node = manager_->node_at(current.index());
        bddvar var = node.var();

        Arc child0 = node.arc0();
        Arc child1 = node.arc1();

        if (remaining.count(var) > 0) {
            remaining.erase(var);
            current = child1;
        } else {
            mpz_class count1 = get_arc_count_gmp(exact_index_cache_.get(), child1);
            order += count1;
            current = child0;
        }
    }

    if (current == ARC_TERMINAL_1 && remaining.empty()) {
        return order.get_str();
    }

    return "-1";
}

std::set<bddvar> ZDD::exact_get_set(const std::string& order_str) const {
    std::set<bddvar> result;

    if (is_zero()) {
        return result;
    }
    if (is_one()) {
        return result;
    }

    mpz_class order(order_str);
    if (order < 0) {
        return result;
    }

    ensure_exact_index();
    if (!exact_index_cache_) {
        return result;
    }

    Arc current = arc_;
    if (current.is_negated()) {
        current = Arc::node(current.index(), false);
    }

    while (!current.is_constant()) {
        const DDNode& node = manager_->node_at(current.index());
        bddvar var = node.var();

        Arc child1 = node.arc1();
        Arc child0 = node.arc0();

        mpz_class count1 = get_arc_count_gmp(exact_index_cache_.get(), child1);

        if (order < count1) {
            result.insert(var);
            current = child1;
        } else {
            order -= count1;
            current = child0;
        }
    }

    return result;
}
#endif

} // namespace sbdd2
