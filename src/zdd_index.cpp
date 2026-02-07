// SAPPOROBDD 2.0 - ZDD Index implementation
// MIT License

#include "sbdd2/zdd.hpp"
#include <queue>
#include <algorithm>

#if defined(SBDD2_HAS_GMP) || defined(SBDD2_HAS_BIGINT)
#include "sbdd2/exact_int.hpp"
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
// SAPPOROBDD convention: higher level = closer to root, level 0 = terminal
static int get_level(DDManager* mgr, Arc arc) {
    if (arc.is_constant()) {
        return 0;
    }
    const DDNode& node = mgr->node_at(arc.index());
    return static_cast<int>(mgr->lev_of_var(node.var()));
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

    // In SAPPOROBDD2, level numbers are (SAPPOROBDD convention):
    // - Higher level numbers are closer to root
    // - Level 0 is terminal
    // - Children have LOWER level numbers than parents

    // First pass: BFS to find min level and collect all nodes
    Arc root = arc_;
    if (root.is_negated()) {
        root = Arc::node(root.index(), false);
    }

    int root_level = get_level(manager_, root);
    int min_level = root_level;

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
            if (child_level < min_level) min_level = child_level;
            bfs_queue.push(child0);
        }

        if (!child1.is_constant() && visited.find(child1) == visited.end()) {
            visited[child1] = true;
            all_nodes.push_back(child1);
            int child_level = get_level(manager_, child1);
            if (child_level < min_level) min_level = child_level;
            bfs_queue.push(child1);
        }
    }

    index_cache_->height = root_level;  // Height is the root level
    index_cache_->min_level = min_level;  // Min level closest to terminal

    // Initialize level_nodes array (level 0 unused, level 1 to root_level used)
    index_cache_->level_nodes.resize(root_level + 1);

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

    // Process from bottom to top (low level number to high level number)
    // Level min_level is closest to terminals, level root_level is at root
    for (int lev = min_level; lev <= root_level; ++lev) {
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

#if defined(SBDD2_HAS_GMP) || defined(SBDD2_HAS_BIGINT)
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

    // In SAPPOROBDD2, level numbers are (SAPPOROBDD convention):
    // - Higher level numbers are closer to root
    // - Level 0 is terminal
    // - Children have LOWER level numbers than parents

    Arc root = arc_;
    if (root.is_negated()) {
        root = Arc::node(root.index(), false);
    }

    int root_level = get_level(manager_, root);
    int min_level = root_level;

    // BFS to find all nodes and min level
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
            if (child_level < min_level) min_level = child_level;
            bfs_queue.push(child0);
        }

        if (!child1.is_constant() && visited.find(child1) == visited.end()) {
            visited[child1] = true;
            all_nodes.push_back(child1);
            int child_level = get_level(manager_, child1);
            if (child_level < min_level) min_level = child_level;
            bfs_queue.push(child1);
        }
    }

    exact_index_cache_->height = root_level;
    exact_index_cache_->min_level = min_level;
    exact_index_cache_->level_nodes.resize(root_level + 1);

    // Organize nodes by level
    for (const Arc& node : all_nodes) {
        int lev = get_level(manager_, node);
        exact_index_cache_->node_to_idx[node] = exact_index_cache_->level_nodes[lev].size();
        exact_index_cache_->level_nodes[lev].push_back(node);
    }

    // Compute counts bottom-up with GMP
    auto get_count = [this](Arc a) -> exact_int_t {
        if (a.is_constant()) {
            return (a == ARC_TERMINAL_1) ? exact_int_t(1) : exact_int_t(0);
        }
        auto it = exact_index_cache_->count_cache.find(a);
        if (it != exact_index_cache_->count_cache.end()) {
            return it->second;
        }
        return exact_int_t(0);
    };

    // Process from bottom to top (low level number to high level number)
    for (int lev = min_level; lev <= root_level; ++lev) {
        for (std::size_t j = 0; j < exact_index_cache_->level_nodes[lev].size(); ++j) {
            Arc node = exact_index_cache_->level_nodes[lev][j];
            Arc child0 = get_child0_zdd(manager_, node);
            Arc child1 = get_child1_zdd(manager_, node);

            exact_int_t count0 = get_count(child0);
            exact_int_t count1 = get_count(child1);

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

#if defined(SBDD2_HAS_GMP) || defined(SBDD2_HAS_BIGINT)
    exact_index_once_flag_.reset(new std::once_flag());
    exact_index_cache_.reset();
#endif
}

bool ZDD::has_index() const {
    return index_cache_ != nullptr;
}

#if defined(SBDD2_HAS_GMP) || defined(SBDD2_HAS_BIGINT)
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

#if defined(SBDD2_HAS_GMP) || defined(SBDD2_HAS_BIGINT)
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
        return exact_int_to_str(it->second);
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

#if defined(SBDD2_HAS_GMP) || defined(SBDD2_HAS_BIGINT)
// Helper: Get GMP count stored for an arc
static exact_int_t get_arc_count_exact(const ZDDExactIndexData* cache, Arc arc) {
    if (arc.is_constant()) {
        return (arc == ARC_TERMINAL_1) ? exact_int_t(1) : exact_int_t(0);
    }
    auto it = cache->count_cache.find(arc);
    if (it != cache->count_cache.end()) {
        return it->second;
    }
    return exact_int_t(0);
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

    exact_int_t order(0);

    while (!current.is_constant()) {
        const DDNode& node = manager_->node_at(current.index());
        bddvar var = node.var();

        Arc child0 = node.arc0();
        Arc child1 = node.arc1();

        if (remaining.count(var) > 0) {
            remaining.erase(var);
            current = child1;
        } else {
            exact_int_t count1 = get_arc_count_exact(exact_index_cache_.get(), child1);
            order += count1;
            current = child0;
        }
    }

    if (current == ARC_TERMINAL_1 && remaining.empty()) {
        return exact_int_to_str(order);
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

    exact_int_t order(order_str);
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

        exact_int_t count1 = get_arc_count_exact(exact_index_cache_.get(), child1);

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

// ============== Weight Optimization Methods ==============

int64_t ZDD::max_weight(const std::vector<int64_t>& weights, std::set<bddvar>& result_set) const {
    result_set.clear();

    // Handle terminal cases
    if (is_zero()) {
        return INT64_MIN;  // No sets available
    }
    if (is_one()) {
        return 0;  // Only empty set, weight = 0
    }

    ensure_index();
    if (!index_cache_) {
        return INT64_MIN;
    }

    // Dynamic programming: compute max weight for each node bottom-up
    // sto[arc] = {max_weight, take_1_child}
    std::unordered_map<Arc, std::pair<int64_t, bool>, ArcHash, ArcEqual> sto;
    sto[ARC_TERMINAL_0] = {INT64_MIN, false};  // 0-terminal is "impossible"
    sto[ARC_TERMINAL_1] = {0, false};          // 1-terminal (empty set) has weight 0

    // Process from bottom (low level) to top (high level)
    int min_level = index_cache_->min_level;
    int root_level = index_cache_->height;
    Arc root = arc_;
    if (root.is_negated()) {
        root = Arc::node(root.index(), false);
    }

    for (int lev = min_level; lev <= root_level; ++lev) {
        for (const Arc& node : index_cache_->level_nodes[lev]) {
            const DDNode& dd_node = manager_->node_at(node.index());
            bddvar var = dd_node.var();
            Arc child0 = dd_node.arc0();
            Arc child1 = dd_node.arc1();

            int64_t weight0 = sto[child0].first;
            int64_t weight1 = sto[child1].first;
            int64_t var_weight = (var < weights.size()) ? weights[var] : 0;

            // For max: compare child0 weight with child1 weight + var_weight
            if (weight1 == INT64_MIN) {
                // 1-child is "impossible", must take 0-child
                sto[node] = {weight0, false};
            } else if (weight0 == INT64_MIN || weight0 < weight1 + var_weight) {
                // Taking 1-child (including var) gives higher weight
                sto[node] = {weight1 + var_weight, true};
            } else {
                // Taking 0-child gives higher or equal weight
                sto[node] = {weight0, false};
            }
        }
    }

    // Trace back to find the optimal set
    Arc current = root;
    while (!current.is_constant()) {
        auto it = sto.find(current);
        if (it == sto.end()) break;

        const DDNode& dd_node = manager_->node_at(current.index());
        bddvar var = dd_node.var();

        if (it->second.second) {
            // Take 1-child
            result_set.insert(var);
            current = dd_node.arc1();
        } else {
            // Take 0-child
            current = dd_node.arc0();
        }
    }

    return sto[root].first;
}

int64_t ZDD::max_weight(const std::vector<int64_t>& weights) const {
    std::set<bddvar> dummy;
    return max_weight(weights, dummy);
}

int64_t ZDD::min_weight(const std::vector<int64_t>& weights, std::set<bddvar>& result_set) const {
    result_set.clear();

    if (is_zero()) {
        return INT64_MAX;  // No sets available
    }
    if (is_one()) {
        return 0;  // Only empty set, weight = 0
    }

    ensure_index();
    if (!index_cache_) {
        return INT64_MAX;
    }

    std::unordered_map<Arc, std::pair<int64_t, bool>, ArcHash, ArcEqual> sto;
    sto[ARC_TERMINAL_0] = {INT64_MAX, false};  // 0-terminal is "impossible"
    sto[ARC_TERMINAL_1] = {0, false};          // 1-terminal (empty set) has weight 0

    int min_level = index_cache_->min_level;
    int root_level = index_cache_->height;
    Arc root = arc_;
    if (root.is_negated()) {
        root = Arc::node(root.index(), false);
    }

    for (int lev = min_level; lev <= root_level; ++lev) {
        for (const Arc& node : index_cache_->level_nodes[lev]) {
            const DDNode& dd_node = manager_->node_at(node.index());
            bddvar var = dd_node.var();
            Arc child0 = dd_node.arc0();
            Arc child1 = dd_node.arc1();

            int64_t weight0 = sto[child0].first;
            int64_t weight1 = sto[child1].first;
            int64_t var_weight = (var < weights.size()) ? weights[var] : 0;

            // For min: compare child0 weight with child1 weight + var_weight
            if (weight1 == INT64_MAX) {
                // 1-child is "impossible", must take 0-child
                sto[node] = {weight0, false};
            } else if (weight0 == INT64_MAX || weight0 > weight1 + var_weight) {
                // Taking 1-child (including var) gives lower weight
                sto[node] = {weight1 + var_weight, true};
            } else {
                // Taking 0-child gives lower or equal weight
                sto[node] = {weight0, false};
            }
        }
    }

    // Trace back to find the optimal set
    Arc current = root;
    while (!current.is_constant()) {
        auto it = sto.find(current);
        if (it == sto.end()) break;

        const DDNode& dd_node = manager_->node_at(current.index());
        bddvar var = dd_node.var();

        if (it->second.second) {
            result_set.insert(var);
            current = dd_node.arc1();
        } else {
            current = dd_node.arc0();
        }
    }

    return sto[root].first;
}

int64_t ZDD::min_weight(const std::vector<int64_t>& weights) const {
    std::set<bddvar> dummy;
    return min_weight(weights, dummy);
}

int64_t ZDD::sum_weight(const std::vector<int64_t>& weights) const {
    if (is_zero()) {
        return 0;
    }
    if (is_one()) {
        return 0;  // Empty set has weight 0
    }

    ensure_index();
    if (!index_cache_) {
        return 0;
    }

    // sto[arc] = sum of weights of all sets in the subtree rooted at arc
    // sto[f] = sto[child0] + sto[child1] + weight[var] * count(child1)
    std::unordered_map<Arc, int64_t, ArcHash, ArcEqual> sto;
    sto[ARC_TERMINAL_0] = 0;
    sto[ARC_TERMINAL_1] = 0;

    int min_level = index_cache_->min_level;
    int root_level = index_cache_->height;
    Arc root = arc_;
    if (root.is_negated()) {
        root = Arc::node(root.index(), false);
    }

    for (int lev = min_level; lev <= root_level; ++lev) {
        for (const Arc& node : index_cache_->level_nodes[lev]) {
            const DDNode& dd_node = manager_->node_at(node.index());
            bddvar var = dd_node.var();
            Arc child0 = dd_node.arc0();
            Arc child1 = dd_node.arc1();

            int64_t sum0 = sto[child0];
            int64_t sum1 = sto[child1];
            int64_t var_weight = (var < weights.size()) ? weights[var] : 0;
            double count1 = get_arc_count(index_cache_.get(), child1);

            // Sum for this node = sum of child subtrees + weight[var] * count of 1-child sets
            sto[node] = sum0 + sum1 + var_weight * static_cast<int64_t>(count1);
        }
    }

    return sto[root];
}

#if defined(SBDD2_HAS_GMP) || defined(SBDD2_HAS_BIGINT)
std::string ZDD::exact_sum_weight(const std::vector<int64_t>& weights) const {
    if (is_zero()) {
        return "0";
    }
    if (is_one()) {
        return "0";
    }

    ensure_exact_index();
    if (!exact_index_cache_) {
        return "0";
    }

    std::unordered_map<Arc, exact_int_t, ArcHash, ArcEqual> sto;
    sto[ARC_TERMINAL_0] = exact_int_t(0);
    sto[ARC_TERMINAL_1] = exact_int_t(0);

    int min_level = exact_index_cache_->min_level;
    int root_level = exact_index_cache_->height;
    Arc root = arc_;
    if (root.is_negated()) {
        root = Arc::node(root.index(), false);
    }

    for (int lev = min_level; lev <= root_level; ++lev) {
        for (const Arc& node : exact_index_cache_->level_nodes[lev]) {
            const DDNode& dd_node = manager_->node_at(node.index());
            bddvar var = dd_node.var();
            Arc child0 = dd_node.arc0();
            Arc child1 = dd_node.arc1();

            exact_int_t sum0 = sto[child0];
            exact_int_t sum1 = sto[child1];
            exact_int_t var_weight = (var < weights.size()) ? exact_int_t(weights[var]) : exact_int_t(0);
            exact_int_t count1 = get_arc_count_exact(exact_index_cache_.get(), child1);

            sto[node] = sum0 + sum1 + var_weight * count1;
        }
    }

    return exact_int_to_str(sto[root]);
}
#endif

} // namespace sbdd2
