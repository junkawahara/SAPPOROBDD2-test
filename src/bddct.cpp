// SAPPOROBDD 2.0 - BDDCT implementation
// MIT License

#include "sbdd2/bddct.hpp"
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdlib>
#include <ctime>
#include <functional>

namespace sbdd2 {

// Constructor
BDDCT::BDDCT()
    : manager_(nullptr)
    , n_vars_(0)
    , cache_entries_(0)
    , cache0_entries_(0)
    , call_count_(0)
{
}

BDDCT::BDDCT(DDManager& mgr)
    : manager_(&mgr)
    , n_vars_(0)
    , cache_entries_(0)
    , cache0_entries_(0)
    , call_count_(0)
{
}

// Destructor
BDDCT::~BDDCT() {
    // CacheEntry destructor handles cleanup
}

// Move constructor
BDDCT::BDDCT(BDDCT&& other) noexcept
    : manager_(other.manager_)
    , n_vars_(other.n_vars_)
    , costs_(std::move(other.costs_))
    , labels_(std::move(other.labels_))
    , cache_(std::move(other.cache_))
    , cache0_(std::move(other.cache0_))
    , cache_entries_(other.cache_entries_)
    , cache0_entries_(other.cache0_entries_)
    , call_count_(other.call_count_)
{
    other.manager_ = nullptr;
    other.n_vars_ = 0;
}

// Move assignment
BDDCT& BDDCT::operator=(BDDCT&& other) noexcept {
    if (this != &other) {
        manager_ = other.manager_;
        n_vars_ = other.n_vars_;
        costs_ = std::move(other.costs_);
        labels_ = std::move(other.labels_);
        cache_ = std::move(other.cache_);
        cache0_ = std::move(other.cache0_);
        cache_entries_ = other.cache_entries_;
        cache0_entries_ = other.cache0_entries_;
        call_count_ = other.call_count_;

        other.manager_ = nullptr;
        other.n_vars_ = 0;
    }
    return *this;
}

// Initialization
bool BDDCT::alloc(int n_vars, bddcost default_cost) {
    if (n_vars < 0) return false;

    n_vars_ = n_vars;
    costs_.resize(n_vars + 1, default_cost);
    labels_.resize(n_vars + 1);

    for (int i = 0; i <= n_vars; i++) {
        labels_[i] = "v" + std::to_string(i);
    }

    // Initialize caches
    cache_.resize(1024);
    cache0_.resize(1024);

    return true;
}

bool BDDCT::import(FILE* fp) {
    if (!fp) return false;

    int n;
    if (fscanf(fp, "%d", &n) != 1) return false;

    alloc(n);

    for (int i = 1; i <= n; i++) {
        int cost;
        char label[CT_STRLEN + 1];
        if (fscanf(fp, "%d %15s", &cost, label) != 2) return false;
        costs_[i] = cost;
        labels_[i] = label;
    }

    return true;
}

bool BDDCT::import(const std::string& filename) {
    FILE* fp = fopen(filename.c_str(), "r");
    if (!fp) return false;
    bool result = import(fp);
    fclose(fp);
    return result;
}

bool BDDCT::alloc_random(int n_vars, bddcost min_cost, bddcost max_cost) {
    if (!alloc(n_vars)) return false;

    std::srand(static_cast<unsigned>(std::time(nullptr)));
    for (int i = 1; i <= n_vars; i++) {
        costs_[i] = min_cost + std::rand() % (max_cost - min_cost + 1);
    }

    return true;
}

// Cost access
bddcost BDDCT::cost(int var_index) const {
    if (var_index < 0 || var_index > n_vars_) return BDDCOST_NULL;
    return costs_[var_index];
}

bddcost BDDCT::cost_of_level(int level) const {
    return cost(n_vars_ - level);
}

bool BDDCT::set_cost(int var_index, bddcost cost) {
    if (var_index < 0 || var_index > n_vars_) return false;
    costs_[var_index] = cost;
    return true;
}

bool BDDCT::set_cost_of_level(int level, bddcost cost) {
    return set_cost(n_vars_ - level, cost);
}

// Label access
static const std::string empty_label;

const std::string& BDDCT::label(int var_index) const {
    if (var_index < 0 || var_index > n_vars_) return empty_label;
    return labels_[var_index];
}

const std::string& BDDCT::label_of_level(int level) const {
    return label(n_vars_ - level);
}

bool BDDCT::set_label(int var_index, const std::string& label) {
    if (var_index < 0 || var_index > n_vars_) return false;
    labels_[var_index] = label;
    return true;
}

bool BDDCT::set_label_of_level(int level, const std::string& label) {
    return set_label(n_vars_ - level, label);
}

// Cache management
void BDDCT::cache_clear() {
    for (auto& entry : cache_) {
        entry.id = ~0ULL;
        delete entry.zmap;
        entry.zmap = nullptr;
    }
    cache_entries_ = 0;
}

void BDDCT::cache_enlarge() {
    std::size_t new_size = cache_.size() * 2;
    cache_.resize(new_size);
}

void BDDCT::cache0_clear() {
    for (auto& entry : cache0_) {
        entry.id = ~0ULL;
        entry.bound = BDDCOST_NULL;
        entry.op = 255;
    }
    cache0_entries_ = 0;
}

void BDDCT::cache0_enlarge() {
    std::size_t new_size = cache0_.size() * 2;
    cache0_.resize(new_size);
}

// Cost-bounded operations
ZDD BDDCT::zdd_cost_le(const ZDD& f, bddcost bound) {
    bddcost aw, rb;
    return zdd_cost_le(f, bound, aw, rb);
}

ZDD BDDCT::zdd_cost_le(const ZDD& f, bddcost bound,
                        bddcost& actual_weight, bddcost& reduced_bound) {
    if (!manager_ || !f.manager()) return ZDD();

    call_count_++;

    if (f.is_zero()) {
        actual_weight = 0;
        reduced_bound = bound;
        return ZDD::empty(*manager_);
    }

    if (f.is_one()) {
        actual_weight = 0;
        reduced_bound = bound;
        return (bound >= 0) ? ZDD::single(*manager_) : ZDD::empty(*manager_);
    }

    // Check cache
    ZDD cached = cache_ref(f, bound, actual_weight, reduced_bound);
    if (cached.is_valid()) {
        return cached;
    }

    bddvar top = f.top();
    bddcost c = cost(static_cast<int>(top));

    ZDD f0 = f.offset(top);
    ZDD f1 = f.onset(top);

    // Process low branch (element not selected)
    bddcost aw0, rb0;
    ZDD z0 = zdd_cost_le(f0, bound, aw0, rb0);

    // Process high branch (element selected, add cost)
    bddcost aw1, rb1;
    ZDD z1;
    if (bound >= c) {
        z1 = zdd_cost_le(f1, bound - c, aw1, rb1);
        aw1 += c;
    } else {
        z1 = ZDD::empty(*manager_);
        aw1 = 0;
        rb1 = bound;
    }

    // Combine results
    ZDD result;
    if (z1.is_zero()) {
        result = z0;
        actual_weight = aw0;
        reduced_bound = rb0;
    } else {
        // Create node with top variable
        Arc arc = manager_->get_or_create_node_zdd(top, z0.arc(), z1.arc(), true);
        result = ZDD(manager_, arc);
        actual_weight = std::max(aw0, aw1);
        reduced_bound = std::min(rb0, rb1 + c);
    }

    // Store in cache
    cache_ent(f, result, bound, actual_weight);

    return result;
}

ZDD BDDCT::zdd_cost_le0(const ZDD& f, bddcost bound) {
    // Simple version without weight tracking
    if (!manager_ || !f.manager()) return ZDD();

    if (f.is_zero()) return ZDD::empty(*manager_);
    if (f.is_one()) return (bound >= 0) ? ZDD::single(*manager_) : ZDD::empty(*manager_);

    bddvar top = f.top();
    bddcost c = cost(static_cast<int>(top));

    ZDD f0 = f.offset(top);
    ZDD f1 = f.onset(top);

    ZDD z0 = zdd_cost_le0(f0, bound);
    ZDD z1 = (bound >= c) ? zdd_cost_le0(f1, bound - c) : ZDD::empty(*manager_);

    if (z1.is_zero()) {
        return z0;
    }

    Arc arc = manager_->get_or_create_node_zdd(top, z0.arc(), z1.arc(), true);
    return ZDD(manager_, arc);
}

// Cost computation
bddcost BDDCT::min_cost(const ZDD& f) {
    if (!manager_ || !f.manager()) return BDDCOST_NULL;
    if (f.is_zero()) return BDDCOST_NULL;
    if (f.is_one()) return 0;

    // Check cache
    bddcost cached = cache0_ref(0, f.id());
    if (cached != BDDCOST_NULL) return cached;

    bddvar top = f.top();
    bddcost c = cost(static_cast<int>(top));

    ZDD f0 = f.offset(top);
    ZDD f1 = f.onset(top);

    bddcost min0 = min_cost(f0);
    bddcost min1 = min_cost(f1);
    if (min1 != BDDCOST_NULL) min1 += c;

    bddcost result;
    if (min0 == BDDCOST_NULL) {
        result = min1;
    } else if (min1 == BDDCOST_NULL) {
        result = min0;
    } else {
        result = std::min(min0, min1);
    }

    cache0_ent(0, f.id(), result);
    return result;
}

bddcost BDDCT::max_cost(const ZDD& f) {
    if (!manager_ || !f.manager()) return BDDCOST_NULL;
    if (f.is_zero()) return BDDCOST_NULL;
    if (f.is_one()) return 0;

    bddcost cached = cache0_ref(1, f.id());
    if (cached != BDDCOST_NULL) return cached;

    bddvar top = f.top();
    bddcost c = cost(static_cast<int>(top));

    ZDD f0 = f.offset(top);
    ZDD f1 = f.onset(top);

    bddcost max0 = max_cost(f0);
    bddcost max1 = max_cost(f1);
    if (max1 != BDDCOST_NULL) max1 += c;

    bddcost result;
    if (max0 == BDDCOST_NULL) {
        result = max1;
    } else if (max1 == BDDCOST_NULL) {
        result = max0;
    } else {
        result = std::max(max0, max1);
    }

    cache0_ent(1, f.id(), result);
    return result;
}

// Cache helpers
ZDD BDDCT::cache_ref(const ZDD& f, bddcost bound, bddcost& aw, bddcost& rb) {
    std::size_t idx = (f.id() * 31 + bound) % cache_.size();
    const CacheEntry& entry = cache_[idx];

    if (entry.id == f.id() && entry.zmap) {
        auto it = entry.zmap->find(bound);
        if (it != entry.zmap->end()) {
            aw = 0;  // Simplified
            rb = bound;
            return it->second;
        }
    }

    return ZDD();  // Invalid - cache miss
}

void BDDCT::cache_ent(const ZDD& f, const ZDD& result, bddcost bound, bddcost cost) {
    std::size_t idx = (f.id() * 31 + bound) % cache_.size();
    CacheEntry& entry = cache_[idx];

    if (entry.id != f.id()) {
        delete entry.zmap;
        entry.zmap = new CostMap();
        entry.id = f.id();
    }

    (*entry.zmap)[bound] = result;
    cache_entries_++;
}

bddcost BDDCT::cache0_ref(std::uint8_t op, std::uint64_t id) const {
    std::size_t idx = (id * 31 + op) % cache0_.size();
    const Cache0Entry& entry = cache0_[idx];

    if (entry.id == id && entry.op == op) {
        return entry.bound;
    }

    return BDDCOST_NULL;
}

void BDDCT::cache0_ent(std::uint8_t op, std::uint64_t id, bddcost result) {
    std::size_t idx = (id * 31 + op) % cache0_.size();
    Cache0Entry& entry = cache0_[idx];

    entry.id = id;
    entry.op = op;
    entry.bound = result;
    cache0_entries_++;
}

// Output
void BDDCT::export_table(FILE* fp) const {
    fprintf(fp, "%d\n", n_vars_);
    for (int i = 1; i <= n_vars_; i++) {
        fprintf(fp, "%d %s\n", costs_[i], labels_[i].c_str());
    }
}

std::string BDDCT::to_string() const {
    std::ostringstream oss;
    oss << "BDDCT(n=" << n_vars_ << ")\n";
    for (int i = 1; i <= n_vars_; i++) {
        oss << "  " << labels_[i] << ": cost=" << costs_[i] << "\n";
    }
    return oss.str();
}

} // namespace sbdd2
