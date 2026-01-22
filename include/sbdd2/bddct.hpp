// SAPPOROBDD 2.0 - BDDCT (BDD Cost Table) class
// MIT License

#ifndef SBDD2_BDDCT_HPP
#define SBDD2_BDDCT_HPP

#include "zdd.hpp"
#include <vector>
#include <string>
#include <map>
#include <cstdio>

namespace sbdd2 {

// Cost type
using bddcost = int;
constexpr bddcost BDDCOST_NULL = 0x7FFFFFFF;
constexpr int CT_STRLEN = 15;

// BDDCT - BDD Cost Table class
// Associates costs with variables for cost-bounded enumeration
class BDDCT {
public:
    // Cost map type: cost -> ZDD of solutions
    using CostMap = std::map<bddcost, ZDD>;

private:
    DDManager* manager_;
    int n_vars_;
    std::vector<bddcost> costs_;
    std::vector<std::string> labels_;

    // Cache structures
    struct CacheEntry {
        std::uint64_t id;
        CostMap* zmap;

        CacheEntry() : id(~0ULL), zmap(nullptr) {}
        ~CacheEntry() { delete zmap; }
    };

    struct Cache0Entry {
        std::uint64_t id;
        bddcost bound;
        std::uint8_t op;

        Cache0Entry() : id(~0ULL), bound(BDDCOST_NULL), op(255) {}
    };

    std::vector<CacheEntry> cache_;
    std::vector<Cache0Entry> cache0_;
    std::size_t cache_entries_;
    std::size_t cache0_entries_;
    std::size_t call_count_;

public:
    // Constructor
    BDDCT();
    explicit BDDCT(DDManager& mgr);

    // Destructor
    ~BDDCT();

    // Non-copyable (due to cache pointers)
    BDDCT(const BDDCT&) = delete;
    BDDCT& operator=(const BDDCT&) = delete;

    // Movable
    BDDCT(BDDCT&&) noexcept;
    BDDCT& operator=(BDDCT&&) noexcept;

    // Initialization
    bool alloc(int n_vars, bddcost default_cost = 1);
    bool import(FILE* fp);
    bool import(const std::string& filename);
    bool alloc_random(int n_vars, bddcost min_cost, bddcost max_cost);

    // Cost access
    bddcost cost(int var_index) const;
    bddcost cost_of_level(int level) const;
    bool set_cost(int var_index, bddcost cost);
    bool set_cost_of_level(int level, bddcost cost);

    // Label access
    const std::string& label(int var_index) const;
    const std::string& label_of_level(int level) const;
    bool set_label(int var_index, const std::string& label);
    bool set_label_of_level(int level, const std::string& label);

    // Query
    int size() const { return n_vars_; }
    DDManager* manager() const { return manager_; }

    // Cost-bounded operations
    ZDD zdd_cost_le(const ZDD& f, bddcost bound);
    ZDD zdd_cost_le(const ZDD& f, bddcost bound, bddcost& actual_weight, bddcost& reduced_bound);
    ZDD zdd_cost_le0(const ZDD& f, bddcost bound);  // Simple version

    // Cost computation
    bddcost min_cost(const ZDD& f);
    bddcost max_cost(const ZDD& f);

    // Cache management
    void cache_clear();
    void cache_enlarge();
    void cache0_clear();
    void cache0_enlarge();

    // Output
    void export_table(FILE* fp = stdout) const;
    std::string to_string() const;

private:
    // Cache helpers
    ZDD cache_ref(const ZDD& f, bddcost bound, bddcost& aw, bddcost& rb);
    void cache_ent(const ZDD& f, const ZDD& result, bddcost bound, bddcost cost);
    bddcost cache0_ref(std::uint8_t op, std::uint64_t id) const;
    void cache0_ent(std::uint8_t op, std::uint64_t id, bddcost result);
};

} // namespace sbdd2

#endif // SBDD2_BDDCT_HPP
