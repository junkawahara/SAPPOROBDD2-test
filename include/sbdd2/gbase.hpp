// SAPPOROBDD 2.0 - GBase (Graph Base) class
// MIT License

#ifndef SBDD2_GBASE_HPP
#define SBDD2_GBASE_HPP

#include "zdd.hpp"
#include <vector>
#include <string>
#include <cstdio>

namespace sbdd2 {

// Type definitions for graph
using GBVertex = std::uint16_t;  // Up to 65534 vertices
using GBEdge = std::uint16_t;    // Up to 65534 edges

// GBase - Graph Base class for path/cycle enumeration
class GBase {
public:
    // Edge constraint flags
    static constexpr char FIX_NONE = 0;
    static constexpr char FIX_0 = 1;  // Edge must NOT be used
    static constexpr char FIX_1 = 2;  // Edge MUST be used

    // Vertex structure
    struct Vertex {
        int tmp;
        GBEdge degree;

        Vertex() : tmp(0), degree(0) {}
    };

    // Edge structure
    struct Edge {
        GBVertex endpoints[2];
        int tmp;
        int cost;
        char io[2];
        char preset;
        GBVertex mate_width;

        Edge() : tmp(0), cost(1), preset(FIX_NONE), mate_width(0) {
            endpoints[0] = endpoints[1] = 0;
            io[0] = io[1] = 0;
        }
    };

private:
    DDManager* manager_;
    GBVertex n_vertices_;
    GBEdge n_edges_;
    std::vector<Vertex> vertices_;
    std::vector<Edge> edges_;
    bool hamilton_mode_;
    ZDD constraint_;
    GBEdge last_in_;

public:
    // Constructor
    GBase();
    explicit GBase(DDManager& mgr);

    // Destructor
    ~GBase() = default;

    // Copy and move
    GBase(const GBase&) = default;
    GBase(GBase&&) noexcept = default;
    GBase& operator=(const GBase&) = default;
    GBase& operator=(GBase&&) noexcept = default;

    // Initialization
    bool init(GBVertex n_vertices, GBEdge n_edges);
    bool set_edge(GBEdge e, GBVertex v1, GBVertex v2, int cost = 1);
    bool import(FILE* fp);
    bool import(const std::string& filename);

    // Grid graph creation
    bool set_grid(int rows, int cols);

    // Pack (remove unused vertices)
    bool pack();

    // Edge constraints
    void fix_edge(GBEdge e, char fix_type);
    void set_hamilton(bool enable);
    void set_constraint(const ZDD& constraint);

    // Path/Cycle enumeration
    ZDD sim_paths(GBVertex start, GBVertex end);
    ZDD sim_cycles();

    // Variable mapping
    int bdd_var_of_edge(GBEdge e) const;
    GBEdge edge_of_bdd_var(int var) const;

    // Query
    GBVertex vertex_count() const { return n_vertices_; }
    GBEdge edge_count() const { return n_edges_; }
    DDManager* manager() const { return manager_; }

    // Output
    void print() const;
    std::string to_string() const;

private:
    // Internal helpers for simpath algorithm
    ZDD simpath_rec(GBEdge e, GBVertex start, GBVertex end,
                    std::vector<GBVertex>& mate);
    void update_mate(std::vector<GBVertex>& mate, GBVertex v1, GBVertex v2);
};

} // namespace sbdd2

#endif // SBDD2_GBASE_HPP
