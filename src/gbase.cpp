// SAPPOROBDD 2.0 - GBase implementation
// MIT License

#include "sbdd2/gbase.hpp"
#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <queue>

namespace sbdd2 {

// Constructor
GBase::GBase()
    : manager_(nullptr)
    , n_vertices_(0)
    , n_edges_(0)
    , hamilton_mode_(false)
    , last_in_(0)
{
}

GBase::GBase(DDManager& mgr)
    : manager_(&mgr)
    , n_vertices_(0)
    , n_edges_(0)
    , hamilton_mode_(false)
    , last_in_(0)
{
}

// Initialization
bool GBase::init(GBVertex n_vertices, GBEdge n_edges) {
    n_vertices_ = n_vertices;
    n_edges_ = n_edges;
    vertices_.resize(n_vertices + 1);
    edges_.resize(n_edges + 1);

    // Create BDD variables for edges
    if (manager_) {
        for (GBEdge e = 1; e <= n_edges; e++) {
            manager_->new_var();
        }
    }

    return true;
}

bool GBase::set_edge(GBEdge e, GBVertex v1, GBVertex v2, int cost) {
    if (e < 1 || e > n_edges_) return false;
    if (v1 < 1 || v1 > n_vertices_) return false;
    if (v2 < 1 || v2 > n_vertices_) return false;

    edges_[e].endpoints[0] = v1;
    edges_[e].endpoints[1] = v2;
    edges_[e].cost = cost;

    vertices_[v1].degree++;
    vertices_[v2].degree++;

    return true;
}

bool GBase::import(FILE* fp) {
    if (!fp) return false;

    int n, m;
    if (fscanf(fp, "%d %d", &n, &m) != 2) return false;

    init(static_cast<GBVertex>(n), static_cast<GBEdge>(m));

    for (int i = 1; i <= m; i++) {
        int v1, v2;
        if (fscanf(fp, "%d %d", &v1, &v2) != 2) return false;
        set_edge(static_cast<GBEdge>(i),
                 static_cast<GBVertex>(v1),
                 static_cast<GBVertex>(v2));
    }

    return true;
}

bool GBase::import(const std::string& filename) {
    FILE* fp = fopen(filename.c_str(), "r");
    if (!fp) return false;
    bool result = import(fp);
    fclose(fp);
    return result;
}

// Grid graph creation
bool GBase::set_grid(int rows, int cols) {
    if (rows < 1 || cols < 1) return false;

    GBVertex n = static_cast<GBVertex>(rows * cols);
    GBEdge m = static_cast<GBEdge>((rows - 1) * cols + rows * (cols - 1));

    init(n, m);

    GBEdge e = 1;

    // Horizontal edges
    for (int r = 0; r < rows; r++) {
        for (int c = 0; c < cols - 1; c++) {
            GBVertex v1 = static_cast<GBVertex>(r * cols + c + 1);
            GBVertex v2 = static_cast<GBVertex>(r * cols + c + 2);
            set_edge(e++, v1, v2);
        }
    }

    // Vertical edges
    for (int r = 0; r < rows - 1; r++) {
        for (int c = 0; c < cols; c++) {
            GBVertex v1 = static_cast<GBVertex>(r * cols + c + 1);
            GBVertex v2 = static_cast<GBVertex>((r + 1) * cols + c + 1);
            set_edge(e++, v1, v2);
        }
    }

    return true;
}

// Pack (remove unused vertices)
bool GBase::pack() {
    // Count actual vertices used
    std::vector<bool> used(n_vertices_ + 1, false);
    for (GBEdge e = 1; e <= n_edges_; e++) {
        used[edges_[e].endpoints[0]] = true;
        used[edges_[e].endpoints[1]] = true;
    }

    // Create mapping
    std::vector<GBVertex> mapping(n_vertices_ + 1, 0);
    GBVertex new_n = 0;
    for (GBVertex v = 1; v <= n_vertices_; v++) {
        if (used[v]) {
            mapping[v] = ++new_n;
        }
    }

    // Update edges
    for (GBEdge e = 1; e <= n_edges_; e++) {
        edges_[e].endpoints[0] = mapping[edges_[e].endpoints[0]];
        edges_[e].endpoints[1] = mapping[edges_[e].endpoints[1]];
    }

    n_vertices_ = new_n;
    vertices_.resize(new_n + 1);

    return true;
}

// Edge constraints
void GBase::fix_edge(GBEdge e, char fix_type) {
    if (e >= 1 && e <= n_edges_) {
        edges_[e].preset = fix_type;
    }
}

void GBase::set_hamilton(bool enable) {
    hamilton_mode_ = enable;
}

void GBase::set_constraint(const ZDD& constraint) {
    constraint_ = constraint;
}

// Variable mapping
int GBase::bdd_var_of_edge(GBEdge e) const {
    return static_cast<int>(e);
}

GBEdge GBase::edge_of_bdd_var(int var) const {
    return static_cast<GBEdge>(var);
}

// Helper: update mate array after selecting an edge
void GBase::update_mate(std::vector<GBVertex>& mate, GBVertex v1, GBVertex v2) {
    GBVertex m1 = mate[v1];
    GBVertex m2 = mate[v2];

    if (m1 == v2) {
        // Cycle detected
        mate[v1] = 0;
        mate[v2] = 0;
    } else {
        // Connect the paths
        if (m1 != 0) mate[m1] = m2;
        if (m2 != 0) mate[m2] = m1;
        mate[v1] = 0;
        mate[v2] = 0;
    }
}

// Simpath algorithm for path enumeration
ZDD GBase::simpath_rec(GBEdge e, GBVertex start, GBVertex end,
                        std::vector<GBVertex>& mate) {
    if (!manager_) return ZDD();

    // Terminal case
    if (e > n_edges_) {
        // Check if valid path
        bool valid = (mate[start] == end || (start == end && mate[start] == 0));

        // Hamilton mode: check all vertices visited
        if (hamilton_mode_) {
            for (GBVertex v = 1; v <= n_vertices_; v++) {
                if (v != start && v != end && mate[v] != 0) {
                    valid = false;
                    break;
                }
            }
        }

        return valid ? ZDD::single(*manager_) : ZDD::empty(*manager_);
    }

    GBVertex v1 = edges_[e].endpoints[0];
    GBVertex v2 = edges_[e].endpoints[1];

    // Check preset
    if (edges_[e].preset == FIX_0) {
        return simpath_rec(e + 1, start, end, mate);
    }

    if (edges_[e].preset == FIX_1) {
        // Must use this edge
        std::vector<GBVertex> new_mate = mate;
        update_mate(new_mate, v1, v2);
        return simpath_rec(e + 1, start, end, new_mate);
    }

    // Branch: not using edge e (0-branch)
    ZDD z0 = simpath_rec(e + 1, start, end, mate);

    // Branch: using edge e (1-branch)
    std::vector<GBVertex> new_mate = mate;
    update_mate(new_mate, v1, v2);

    // Check for invalid cycle (not connecting start-end)
    bool valid = true;
    if (mate[v1] == v2 && !(v1 == start && v2 == end) && !(v1 == end && v2 == start)) {
        // Premature cycle
        valid = false;
    }

    ZDD z1;
    if (valid) {
        z1 = simpath_rec(e + 1, start, end, new_mate);
        if (!z1.is_zero()) {
            bddvar var = static_cast<bddvar>(e);
            Arc arc = manager_->get_or_create_node_zdd(var, ARC_TERMINAL_0, z1.arc(), true);
            z1 = ZDD(manager_, arc);
        }
    } else {
        z1 = ZDD::empty(*manager_);
    }

    return z0 + z1;
}

// Path enumeration
ZDD GBase::sim_paths(GBVertex start, GBVertex end) {
    if (!manager_) return ZDD();
    if (start < 1 || start > n_vertices_) return ZDD::empty(*manager_);
    if (end < 1 || end > n_vertices_) return ZDD::empty(*manager_);

    // Initialize mate array
    std::vector<GBVertex> mate(n_vertices_ + 1);
    for (GBVertex v = 1; v <= n_vertices_; v++) {
        mate[v] = v;  // Each vertex is its own endpoint initially
    }
    mate[start] = end;  // Connect start and end
    mate[end] = start;

    return simpath_rec(1, start, end, mate);
}

// Cycle enumeration
ZDD GBase::sim_cycles() {
    if (!manager_) return ZDD();

    // Find all simple cycles by enumerating paths from each vertex to itself
    ZDD result = ZDD::empty(*manager_);

    for (GBVertex v = 1; v <= n_vertices_; v++) {
        ZDD cycles_v = sim_paths(v, v);
        result = result + cycles_v;
    }

    return result;
}

// Output
void GBase::print() const {
    std::cout << to_string() << std::endl;
}

std::string GBase::to_string() const {
    std::ostringstream oss;
    oss << "GBase(V=" << n_vertices_ << ", E=" << n_edges_ << ")\n";
    for (GBEdge e = 1; e <= n_edges_; e++) {
        oss << "  Edge " << e << ": " << edges_[e].endpoints[0]
            << " -- " << edges_[e].endpoints[1];
        if (edges_[e].preset == FIX_0) oss << " [fixed 0]";
        if (edges_[e].preset == FIX_1) oss << " [fixed 1]";
        oss << "\n";
    }
    return oss.str();
}

} // namespace sbdd2
