// SAPPOROBDD 2.0 - DD Manager implementation
// MIT License

#include "sbdd2/dd_manager.hpp"
#include "sbdd2/bdd.hpp"
#include "sbdd2/zdd.hpp"
#include <algorithm>
#include <cmath>

namespace sbdd2 {

// Constructor
DDManager::DDManager(std::size_t node_table_size, std::size_t cache_size)
    : table_size_(node_table_size)
    , node_count_(0)
    , alive_count_(0)
    , cache_size_(cache_size)
    , var_count_(0)
    , gc_threshold_(0.75)
    , gc_min_nodes_(1000)
{
    // Ensure table size is power of 2
    table_size_ = 1;
    while (table_size_ < node_table_size) {
        table_size_ <<= 1;
    }

    // Ensure cache size is power of 2
    cache_size_ = 1;
    while (cache_size_ < cache_size) {
        cache_size_ <<= 1;
    }

    // Allocate tables
    nodes_.resize(table_size_);
    cache_.resize(cache_size_);
}

// Destructor
DDManager::~DDManager() = default;

// Move constructor
DDManager::DDManager(DDManager&& other) noexcept
    : nodes_(std::move(other.nodes_))
    , table_size_(other.table_size_)
    , node_count_(other.node_count_)
    , alive_count_(other.alive_count_)
    , avail_(std::move(other.avail_))
    , cache_(std::move(other.cache_))
    , cache_size_(other.cache_size_)
    , var_count_(other.var_count_.load())
    , gc_threshold_(other.gc_threshold_)
    , gc_min_nodes_(other.gc_min_nodes_)
{
    other.table_size_ = 0;
    other.node_count_ = 0;
    other.alive_count_ = 0;
    other.cache_size_ = 0;
    other.var_count_ = 0;
}

// Move assignment
DDManager& DDManager::operator=(DDManager&& other) noexcept {
    if (this != &other) {
        nodes_ = std::move(other.nodes_);
        table_size_ = other.table_size_;
        node_count_ = other.node_count_;
        alive_count_ = other.alive_count_;
        avail_ = std::move(other.avail_);
        cache_ = std::move(other.cache_);
        cache_size_ = other.cache_size_;
        var_count_ = other.var_count_.load();
        gc_threshold_ = other.gc_threshold_;
        gc_min_nodes_ = other.gc_min_nodes_;

        other.table_size_ = 0;
        other.node_count_ = 0;
        other.alive_count_ = 0;
        other.cache_size_ = 0;
        other.var_count_ = 0;
    }
    return *this;
}

// Variable management
bddvar DDManager::new_var() {
    return ++var_count_;
}

// Hash function for node lookup
std::size_t DDManager::hash_node(bddvar var, Arc arc0, Arc arc1) const {
    // FNV-1a style hash
    std::size_t hash = 14695981039346656037ULL;
    hash ^= var;
    hash *= 1099511628211ULL;
    hash ^= arc0.data;
    hash *= 1099511628211ULL;
    hash ^= arc1.data;
    hash *= 1099511628211ULL;
    return hash;
}

// Find existing node (returns BDDINDEX_MAX if not found)
bddindex DDManager::find_node(bddvar var, Arc arc0, Arc arc1) const {
    std::size_t hash = hash_node(var, arc0, arc1);
    std::size_t mask = table_size_ - 1;

    for (std::size_t i = 0; i < table_size_; ++i) {
        // Quadratic probing: hash + i^2
        std::size_t idx = (hash + i * i) & mask;
        const DDNode& node = nodes_[idx];

        if (node.is_empty()) {
            return BDDINDEX_MAX;
        }
        if (!node.is_tombstone() && node.equals(arc0, arc1, var)) {
            return static_cast<bddindex>(idx);
        }
    }
    return BDDINDEX_MAX;
}

// Insert node (returns index)
bddindex DDManager::insert_node(bddvar var, Arc arc0, Arc arc1, bool reduced) {
    std::size_t hash = hash_node(var, arc0, arc1);
    std::size_t mask = table_size_ - 1;
    bddindex tombstone_idx = BDDINDEX_MAX;

    for (std::size_t i = 0; i < table_size_; ++i) {
        std::size_t idx = (hash + i * i) & mask;
        DDNode& node = nodes_[idx];

        if (node.is_empty()) {
            // Use first tombstone if found, otherwise use empty slot
            if (tombstone_idx != BDDINDEX_MAX) {
                idx = tombstone_idx;
            }
            nodes_[idx] = DDNode(arc0, arc1, var, reduced, 1);
            ++node_count_;
            ++alive_count_;
            return static_cast<bddindex>(idx);
        }
        if (node.is_tombstone() && tombstone_idx == BDDINDEX_MAX) {
            tombstone_idx = idx;
        }
        if (!node.is_tombstone() && node.equals(arc0, arc1, var)) {
            // Node already exists
            node.inc_refcount();
            if (node.refcount() == 1) {
                ++alive_count_;
            }
            return static_cast<bddindex>(idx);
        }
    }

    // Table is full - should not happen with proper load management
    throw DDMemoryException("Node table is full");
}

// Get or create BDD node
Arc DDManager::get_or_create_node_bdd(bddvar var, Arc arc0, Arc arc1, bool reduced) {
    // BDD reduction rule: if both arcs point to same location, return that arc
    // But we need to handle negation edges
    if (arc0.data == arc1.data) {
        return arc0;
    }

    // Normalize: ensure 1-arc is not negated (use negation on entire result)
    bool result_negated = false;
    if (arc1.is_negated()) {
        arc0 = arc0.negated();
        arc1 = arc1.negated();
        result_negated = true;
    }

    std::lock_guard<std::mutex> lock(table_mutex_);

    // Check load factor
    if (load_factor() > gc_threshold_) {
        gc();
        if (load_factor() > gc_threshold_) {
            resize_table();
        }
    }

    bddindex idx = find_node(var, arc0, arc1);
    if (idx != BDDINDEX_MAX) {
        nodes_[idx].inc_refcount();
        if (nodes_[idx].refcount() == 1) {
            ++alive_count_;
        }
        Arc result = Arc::node(idx, false);
        return result_negated ? result.negated() : result;
    }

    idx = insert_node(var, arc0, arc1, reduced);
    Arc result = Arc::node(idx, false);
    return result_negated ? result.negated() : result;
}

// Get or create ZDD node
Arc DDManager::get_or_create_node_zdd(bddvar var, Arc arc0, Arc arc1, bool reduced) {
    // ZDD reduction rule: if 1-arc points to terminal 0, return 0-arc
    if (arc1 == ARC_TERMINAL_0) {
        return arc0;
    }

    std::lock_guard<std::mutex> lock(table_mutex_);

    if (load_factor() > gc_threshold_) {
        gc();
        if (load_factor() > gc_threshold_) {
            resize_table();
        }
    }

    bddindex idx = find_node(var, arc0, arc1);
    if (idx != BDDINDEX_MAX) {
        nodes_[idx].inc_refcount();
        if (nodes_[idx].refcount() == 1) {
            ++alive_count_;
        }
        return Arc::node(idx, false);
    }

    idx = insert_node(var, arc0, arc1, reduced);
    return Arc::node(idx, false);
}

// Reference counting
void DDManager::inc_ref(Arc arc) {
    if (arc.is_constant()) return;

    std::lock_guard<std::mutex> lock(table_mutex_);
    bddindex idx = arc.index();
    if (idx < table_size_) {
        DDNode& node = nodes_[idx];
        if (node.refcount() == 0) {
            ++alive_count_;
        }
        node.inc_refcount();
    }
}

void DDManager::dec_ref(Arc arc) {
    if (arc.is_constant()) return;

    std::lock_guard<std::mutex> lock(table_mutex_);
    bddindex idx = arc.index();
    if (idx < table_size_) {
        DDNode& node = nodes_[idx];
        if (node.dec_refcount()) {
            --alive_count_;
            // Don't delete immediately - GC will clean up
        }
    }
}

// Node access
const DDNode& DDManager::node_at(bddindex index) const {
    return nodes_[index];
}

DDNode& DDManager::node_at(bddindex index) {
    return nodes_[index];
}

// Load factor
double DDManager::load_factor() const {
    return static_cast<double>(node_count_) / static_cast<double>(table_size_);
}

// Garbage collection
void DDManager::gc() {
    std::lock_guard<std::mutex> lock(table_mutex_);
    mark_and_sweep();
}

void DDManager::gc_if_needed() {
    if (load_factor() > gc_threshold_ && node_count_ > gc_min_nodes_) {
        gc();
    }
}

void DDManager::mark_and_sweep() {
    // Mark all nodes that are reachable from alive nodes
    std::vector<bool> marked(table_size_, false);

    // Mark all alive nodes and their descendants
    for (std::size_t i = 0; i < table_size_; ++i) {
        const DDNode& node = nodes_[i];
        if (!node.is_empty() && !node.is_tombstone() && node.refcount() > 0) {
            mark_arc(Arc::node(i, false), marked);
        }
    }

    // Sweep: mark dead nodes as tombstones
    std::size_t swept = 0;
    for (std::size_t i = 0; i < table_size_; ++i) {
        DDNode& node = nodes_[i];
        if (!node.is_empty() && !node.is_tombstone() && !marked[i]) {
            node.mark_tombstone();
            ++swept;
        }
    }

    node_count_ -= swept;
    cache_clear();
}

void DDManager::mark_arc(Arc arc, std::vector<bool>& marked) {
    if (arc.is_constant()) return;

    bddindex idx = arc.index();
    if (idx >= table_size_ || marked[idx]) return;

    marked[idx] = true;
    const DDNode& node = nodes_[idx];
    mark_arc(node.arc0(), marked);
    mark_arc(node.arc1(), marked);
}

// Resize table
void DDManager::resize_table() {
    std::size_t new_size = table_size_ * 2;
    std::vector<DDNode> new_nodes(new_size);

    // Rehash all nodes
    std::size_t mask = new_size - 1;
    for (std::size_t i = 0; i < table_size_; ++i) {
        const DDNode& node = nodes_[i];
        if (node.is_empty() || node.is_tombstone()) continue;

        std::size_t hash = hash_node(node.var(), node.arc0(), node.arc1());
        for (std::size_t j = 0; j < new_size; ++j) {
            std::size_t idx = (hash + j * j) & mask;
            if (new_nodes[idx].is_empty()) {
                new_nodes[idx] = node;
                break;
            }
        }
    }

    nodes_ = std::move(new_nodes);
    table_size_ = new_size;
}

// Cache operations
bool DDManager::cache_lookup(CacheOp op, Arc f, Arc g, Arc& result) const {
    std::uint64_t key1 = (f.data << 8) | static_cast<std::uint64_t>(op);
    std::uint64_t key2 = g.data;

    std::size_t hash = (key1 * 1099511628211ULL) ^ key2;
    std::size_t idx = hash & (cache_size_ - 1);

    std::lock_guard<std::mutex> lock(cache_mutex_);
    const CacheEntry& entry = cache_[idx];
    if (entry.key1 == key1 && entry.key2 == key2) {
        result = Arc(entry.result);
        return true;
    }
    return false;
}

bool DDManager::cache_lookup3(CacheOp op, Arc f, Arc g, Arc h, Arc& result) const {
    std::uint64_t key1 = (f.data << 8) | static_cast<std::uint64_t>(op);
    std::uint64_t key2 = (g.data << 32) | (h.data & 0xFFFFFFFF);

    std::size_t hash = (key1 * 1099511628211ULL) ^ key2;
    std::size_t idx = hash & (cache_size_ - 1);

    std::lock_guard<std::mutex> lock(cache_mutex_);
    const CacheEntry& entry = cache_[idx];
    if (entry.key1 == key1 && entry.key2 == key2) {
        result = Arc(entry.result);
        return true;
    }
    return false;
}

void DDManager::cache_insert(CacheOp op, Arc f, Arc g, Arc result) {
    std::uint64_t key1 = (f.data << 8) | static_cast<std::uint64_t>(op);
    std::uint64_t key2 = g.data;

    std::size_t hash = (key1 * 1099511628211ULL) ^ key2;
    std::size_t idx = hash & (cache_size_ - 1);

    std::lock_guard<std::mutex> lock(cache_mutex_);
    cache_[idx].key1 = key1;
    cache_[idx].key2 = key2;
    cache_[idx].result = result.data;
}

void DDManager::cache_insert3(CacheOp op, Arc f, Arc g, Arc h, Arc result) {
    std::uint64_t key1 = (f.data << 8) | static_cast<std::uint64_t>(op);
    std::uint64_t key2 = (g.data << 32) | (h.data & 0xFFFFFFFF);

    std::size_t hash = (key1 * 1099511628211ULL) ^ key2;
    std::size_t idx = hash & (cache_size_ - 1);

    std::lock_guard<std::mutex> lock(cache_mutex_);
    cache_[idx].key1 = key1;
    cache_[idx].key2 = key2;
    cache_[idx].result = result.data;
}

void DDManager::cache_clear() {
    std::lock_guard<std::mutex> lock(cache_mutex_);
    for (auto& entry : cache_) {
        entry.clear();
    }
}

// Terminal BDD/ZDD
BDD DDManager::bdd_zero() {
    return BDD(this, ARC_TERMINAL_0);
}

BDD DDManager::bdd_one() {
    return BDD(this, ARC_TERMINAL_1);
}

ZDD DDManager::zdd_empty() {
    return ZDD(this, ARC_TERMINAL_0);
}

ZDD DDManager::zdd_base() {
    return ZDD(this, ARC_TERMINAL_1);
}

// Variable BDD/ZDD
BDD DDManager::var_bdd(bddvar v) {
    if (v == 0 || v > var_count_) {
        throw DDArgumentException("Invalid variable number");
    }
    Arc arc = get_or_create_node_bdd(v, ARC_TERMINAL_0, ARC_TERMINAL_1, true);
    return BDD(this, arc);
}

ZDD DDManager::var_zdd(bddvar v) {
    if (v == 0 || v > var_count_) {
        throw DDArgumentException("Invalid variable number");
    }
    Arc arc = get_or_create_node_zdd(v, ARC_TERMINAL_0, ARC_TERMINAL_1, true);
    return ZDD(this, arc);
}

// Global default manager
static DDManager* g_default_manager = nullptr;

DDManager& default_manager() {
    if (!g_default_manager) {
        g_default_manager = new DDManager();
    }
    return *g_default_manager;
}

} // namespace sbdd2
