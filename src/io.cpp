// SAPPOROBDD 2.0 - Import/Export implementation
// MIT License

#include "sbdd2/io.hpp"
#include <cstring>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <stack>
#include <algorithm>
#include <map>
#include <tuple>

namespace sbdd2 {

// Detect format from filename extension
DDFileFormat detect_format(const std::string& filename) {
    std::size_t dot = filename.rfind('.');
    if (dot == std::string::npos) {
        return DDFileFormat::BINARY;
    }

    std::string ext = filename.substr(dot + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

    if (ext == "bdd" || ext == "zdd" || ext == "bin") {
        return DDFileFormat::BINARY;
    } else if (ext == "txt" || ext == "text") {
        return DDFileFormat::TEXT;
    } else if (ext == "dot" || ext == "gv") {
        return DDFileFormat::DOT;
    } else if (ext == "knuth") {
        return DDFileFormat::KNUTH;
    }

    return DDFileFormat::BINARY;
}

// ============== Binary Export ==============

static bool write_binary_header(std::ostream& os, std::uint8_t type,
                                 std::uint64_t node_count) {
    os.write(DD_MAGIC, 4);
    std::uint16_t version = DD_VERSION;
    os.write(reinterpret_cast<const char*>(&version), 2);
    os.write(reinterpret_cast<const char*>(&type), 1);
    std::uint8_t flags = 0;
    os.write(reinterpret_cast<const char*>(&flags), 1);
    os.write(reinterpret_cast<const char*>(&node_count), 8);
    return os.good();
}

static bool write_binary_node(std::ostream& os, bddvar var, Arc low, Arc high) {
    std::uint32_t v = var;
    os.write(reinterpret_cast<const char*>(&v), 4);
    os.write(reinterpret_cast<const char*>(&low.data), 8);
    os.write(reinterpret_cast<const char*>(&high.data), 8);
    return os.good();
}

template<typename DD>
static bool export_dd_binary(const DD& dd, std::ostream& os, std::uint8_t type) {
    if (!dd.manager()) return false;

    DDManager* mgr = dd.manager();

    // Collect all nodes
    std::unordered_set<bddindex> visited;
    std::vector<bddindex> nodes;
    std::stack<Arc> stack;
    stack.push(dd.arc());

    while (!stack.empty()) {
        Arc a = stack.top();
        stack.pop();

        if (a.is_constant()) continue;

        bddindex idx = a.index();
        if (visited.count(idx)) continue;
        visited.insert(idx);
        nodes.push_back(idx);

        const DDNode& node = mgr->node_at(idx);
        stack.push(node.arc0());
        stack.push(node.arc1());
    }

    // Sort by index for deterministic output
    std::sort(nodes.begin(), nodes.end());

    // Create index mapping
    std::unordered_map<bddindex, std::uint64_t> index_map;
    for (std::size_t i = 0; i < nodes.size(); i++) {
        index_map[nodes[i]] = i + 1;  // 1-indexed in file
    }

    // Write header
    if (!write_binary_header(os, type, nodes.size())) {
        return false;
    }

    // Helper to remap arc
    auto remap_arc = [&](Arc a) -> std::uint64_t {
        if (a.is_constant()) {
            return a.data;
        }
        bddindex new_idx = index_map[a.index()];
        return (new_idx << 2) | (a.data & 3);
    };

    // Write nodes
    for (bddindex idx : nodes) {
        const DDNode& node = mgr->node_at(idx);
        Arc low(remap_arc(node.arc0()));
        Arc high(remap_arc(node.arc1()));
        if (!write_binary_node(os, node.var(), low, high)) {
            return false;
        }
    }

    // Write root
    std::uint64_t root = remap_arc(dd.arc());
    os.write(reinterpret_cast<const char*>(&root), 8);

    return os.good();
}

// ============== Binary Import ==============

static bool read_binary_header(std::istream& is, std::uint8_t& type,
                                std::uint64_t& node_count) {
    char magic[4];
    is.read(magic, 4);
    if (std::memcmp(magic, DD_MAGIC, 4) != 0) {
        return false;
    }

    std::uint16_t version;
    is.read(reinterpret_cast<char*>(&version), 2);
    if (version > DD_VERSION) {
        return false;  // Unknown version
    }

    is.read(reinterpret_cast<char*>(&type), 1);
    std::uint8_t flags;
    is.read(reinterpret_cast<char*>(&flags), 1);
    is.read(reinterpret_cast<char*>(&node_count), 8);

    return is.good();
}

template<typename DD>
static DD import_dd_binary(DDManager& mgr, std::istream& is, std::uint8_t expected_type) {
    std::uint8_t type;
    std::uint64_t node_count;

    if (!read_binary_header(is, type, node_count)) {
        return DD();
    }

    if (type != expected_type) {
        return DD();  // Type mismatch
    }

    // Read nodes and create mapping
    std::vector<Arc> arc_map(node_count + 1);
    arc_map[0] = ARC_TERMINAL_0;

    for (std::uint64_t i = 0; i < node_count; i++) {
        std::uint32_t var;
        std::uint64_t low_data, high_data;

        is.read(reinterpret_cast<char*>(&var), 4);
        is.read(reinterpret_cast<char*>(&low_data), 8);
        is.read(reinterpret_cast<char*>(&high_data), 8);

        if (!is.good()) return DD();

        // Remap arcs
        auto remap = [&](std::uint64_t data) -> Arc {
            if ((data & 2) != 0) {
                // Constant
                return Arc(data);
            }
            bddindex idx = data >> 2;
            if (idx > 0 && idx <= arc_map.size()) {
                Arc base = arc_map[idx];
                return (data & 1) ? base.negated() : base;
            }
            return Arc(data);
        };

        Arc low = remap(low_data);
        Arc high = remap(high_data);

        // Create node
        Arc arc;
        if (expected_type == DD_TYPE_BDD) {
            arc = mgr.get_or_create_node_bdd(var, low, high, true);
        } else {
            arc = mgr.get_or_create_node_zdd(var, low, high, true);
        }
        arc_map[i + 1] = arc;
    }

    // Read root
    std::uint64_t root_data;
    is.read(reinterpret_cast<char*>(&root_data), 8);

    Arc root;
    if ((root_data & 2) != 0) {
        root = Arc(root_data);
    } else {
        bddindex idx = root_data >> 2;
        if (idx > 0 && idx <= arc_map.size()) {
            root = arc_map[idx];
            if (root_data & 1) root = root.negated();
        }
    }

    return DD(&mgr, root);
}

// ============== Text Export ==============

template<typename DD>
static bool export_dd_text(const DD& dd, std::ostream& os, const char* type_name) {
    if (!dd.manager()) return false;

    DDManager* mgr = dd.manager();

    os << type_name << "\n";

    if (dd.is_terminal()) {
        os << "0\n";
        os << (dd.is_one() ? "T1" : "T0") << "\n";
        return os.good();
    }

    // Collect nodes
    std::unordered_set<bddindex> visited;
    std::vector<bddindex> nodes;
    std::stack<Arc> stack;
    stack.push(dd.arc());

    while (!stack.empty()) {
        Arc a = stack.top();
        stack.pop();

        if (a.is_constant()) continue;

        bddindex idx = a.index();
        if (visited.count(idx)) continue;
        visited.insert(idx);
        nodes.push_back(idx);

        const DDNode& node = mgr->node_at(idx);
        stack.push(node.arc0());
        stack.push(node.arc1());
    }

    std::sort(nodes.begin(), nodes.end());

    // Create index mapping
    std::unordered_map<bddindex, std::uint64_t> index_map;
    for (std::size_t i = 0; i < nodes.size(); i++) {
        index_map[nodes[i]] = i + 1;
    }

    os << nodes.size() << "\n";

    // Helper to format arc
    auto format_arc = [&](Arc a) -> std::string {
        std::string result;
        if (a.is_negated()) result = "~";

        if (a.is_constant()) {
            result += a.terminal_value() ? "T1" : "T0";
        } else {
            result += std::to_string(index_map[a.index()]);
        }
        return result;
    };

    // Write nodes
    for (std::size_t i = 0; i < nodes.size(); i++) {
        const DDNode& node = mgr->node_at(nodes[i]);
        os << (i + 1) << " " << node.var() << " "
           << format_arc(node.arc0()) << " "
           << format_arc(node.arc1()) << "\n";
    }

    // Write root
    os << format_arc(dd.arc()) << "\n";

    return os.good();
}

// ============== DOT Export ==============

std::string to_dot(const BDD& bdd, const std::string& name) {
    if (!bdd.manager()) return "";

    DDManager* mgr = bdd.manager();
    std::ostringstream os;

    os << "digraph " << name << " {\n";
    os << "  rankdir=TB;\n";
    os << "  node [shape=circle];\n";

    // Terminal nodes
    os << "  T0 [shape=box, label=\"0\"];\n";
    os << "  T1 [shape=box, label=\"1\"];\n";

    if (bdd.is_terminal()) {
        os << "  root -> " << (bdd.is_one() ? "T1" : "T0") << ";\n";
        os << "}\n";
        return os.str();
    }

    // Collect nodes
    std::unordered_set<bddindex> visited;
    std::stack<Arc> stack;
    stack.push(bdd.arc());

    while (!stack.empty()) {
        Arc a = stack.top();
        stack.pop();

        if (a.is_constant()) continue;

        bddindex idx = a.index();
        if (visited.count(idx)) continue;
        visited.insert(idx);

        const DDNode& node = mgr->node_at(idx);

        os << "  n" << idx << " [label=\"x" << node.var() << "\"];\n";

        // Low edge (dashed)
        if (node.arc0().is_constant()) {
            os << "  n" << idx << " -> "
               << (node.arc0().terminal_value() ? "T1" : "T0")
               << " [style=dashed];\n";
        } else {
            os << "  n" << idx << " -> n" << node.arc0().index()
               << " [style=dashed];\n";
        }

        // High edge (solid)
        if (node.arc1().is_constant()) {
            os << "  n" << idx << " -> "
               << (node.arc1().terminal_value() ? "T1" : "T0") << ";\n";
        } else {
            os << "  n" << idx << " -> n" << node.arc1().index() << ";\n";
        }

        stack.push(node.arc0());
        stack.push(node.arc1());
    }

    // Root
    os << "  root [shape=none, label=\"\"];\n";
    os << "  root -> n" << bdd.arc().index();
    if (bdd.arc().is_negated()) {
        os << " [label=\"~\"]";
    }
    os << ";\n";

    os << "}\n";
    return os.str();
}

std::string to_dot(const ZDD& zdd, const std::string& name) {
    if (!zdd.manager()) return "";

    DDManager* mgr = zdd.manager();
    std::ostringstream os;

    os << "digraph " << name << " {\n";
    os << "  rankdir=TB;\n";
    os << "  node [shape=circle];\n";
    os << "  empty [shape=box, label=\"{}\"];\n";
    os << "  base [shape=box, label=\"{{}}\"];\n";

    if (zdd.is_terminal()) {
        os << "  root -> " << (zdd.is_one() ? "base" : "empty") << ";\n";
        os << "}\n";
        return os.str();
    }

    std::unordered_set<bddindex> visited;
    std::stack<Arc> stack;
    stack.push(zdd.arc());

    while (!stack.empty()) {
        Arc a = stack.top();
        stack.pop();

        if (a.is_constant()) continue;

        bddindex idx = a.index();
        if (visited.count(idx)) continue;
        visited.insert(idx);

        const DDNode& node = mgr->node_at(idx);

        os << "  n" << idx << " [label=\"x" << node.var() << "\"];\n";

        // Low edge
        if (node.arc0().is_constant()) {
            os << "  n" << idx << " -> "
               << (node.arc0().terminal_value() ? "base" : "empty")
               << " [style=dashed];\n";
        } else {
            os << "  n" << idx << " -> n" << node.arc0().index()
               << " [style=dashed];\n";
        }

        // High edge
        if (node.arc1().is_constant()) {
            os << "  n" << idx << " -> "
               << (node.arc1().terminal_value() ? "base" : "empty") << ";\n";
        } else {
            os << "  n" << idx << " -> n" << node.arc1().index() << ";\n";
        }

        stack.push(node.arc0());
        stack.push(node.arc1());
    }

    os << "  root [shape=none, label=\"\"];\n";
    os << "  root -> n" << zdd.arc().index() << ";\n";
    os << "}\n";

    return os.str();
}

// ============== Public API ==============

bool export_bdd(const BDD& bdd, const std::string& filename,
                const ExportOptions& options) {
    std::ofstream ofs(filename, std::ios::binary);
    if (!ofs) return false;
    return export_bdd(bdd, ofs, options);
}

bool export_bdd(const BDD& bdd, std::ostream& os, const ExportOptions& options) {
    DDFileFormat fmt = options.format;

    switch (fmt) {
    case DDFileFormat::BINARY:
        return export_dd_binary(bdd, os, DD_TYPE_BDD);
    case DDFileFormat::TEXT:
        return export_dd_text(bdd, os, "BDD");
    case DDFileFormat::DOT:
        os << to_dot(bdd);
        return os.good();
    default:
        return export_dd_binary(bdd, os, DD_TYPE_BDD);
    }
}

BDD import_bdd(DDManager& mgr, const std::string& filename,
               const ImportOptions& options) {
    std::ifstream ifs(filename, std::ios::binary);
    if (!ifs) return BDD();
    return import_bdd(mgr, ifs, options);
}

BDD import_bdd(DDManager& mgr, std::istream& is, const ImportOptions& options) {
    return import_dd_binary<BDD>(mgr, is, DD_TYPE_BDD);
}

bool export_zdd(const ZDD& zdd, const std::string& filename,
                const ExportOptions& options) {
    std::ofstream ofs(filename, std::ios::binary);
    if (!ofs) return false;
    return export_zdd(zdd, ofs, options);
}

bool export_zdd(const ZDD& zdd, std::ostream& os, const ExportOptions& options) {
    DDFileFormat fmt = options.format;

    switch (fmt) {
    case DDFileFormat::BINARY:
        return export_dd_binary(zdd, os, DD_TYPE_ZDD);
    case DDFileFormat::TEXT:
        return export_dd_text(zdd, os, "ZDD");
    case DDFileFormat::DOT:
        os << to_dot(zdd);
        return os.good();
    default:
        return export_dd_binary(zdd, os, DD_TYPE_ZDD);
    }
}

ZDD import_zdd(DDManager& mgr, const std::string& filename,
               const ImportOptions& options) {
    std::ifstream ifs(filename, std::ios::binary);
    if (!ifs) return ZDD();
    return import_zdd(mgr, ifs, options);
}

ZDD import_zdd(DDManager& mgr, std::istream& is, const ImportOptions& options) {
    return import_dd_binary<ZDD>(mgr, is, DD_TYPE_ZDD);
}

// Validation
bool validate_bdd(const BDD& bdd) {
    if (!bdd.manager()) return false;
    // Basic validation: can traverse without errors
    try {
        bdd.size();
        return true;
    } catch (...) {
        return false;
    }
}

bool validate_zdd(const ZDD& zdd) {
    if (!zdd.manager()) return false;
    try {
        zdd.size();
        return true;
    } catch (...) {
        return false;
    }
}

// ============== Graphillion Format ==============
// Format: Lines of "node_id lo_id hi_id" with optional "B" header
// Nodes are 1-indexed, 0 is terminal-0, -1 is terminal-1

ZDD import_zdd_as_graphillion(DDManager& mgr, std::istream& is, int root_level) {
    std::string line;
    std::vector<std::tuple<int, int, int>> nodes;  // (lo, hi) pairs indexed by node_id
    int max_node_id = 0;

    // Skip header if present
    if (std::getline(is, line)) {
        if (line.empty() || line[0] == 'B' || line[0] == '#') {
            // Header line, continue
        } else {
            // First data line
            int id, lo, hi;
            std::istringstream iss(line);
            if (iss >> id >> lo >> hi) {
                nodes.push_back(std::make_tuple(id, lo, hi));
                max_node_id = std::max(max_node_id, id);
            }
        }
    }

    // Read remaining lines
    while (std::getline(is, line)) {
        if (line.empty() || line[0] == '#') continue;

        int id, lo, hi;
        std::istringstream iss(line);
        if (iss >> id >> lo >> hi) {
            nodes.push_back(std::make_tuple(id, lo, hi));
            max_node_id = std::max(max_node_id, id);
        }
    }

    if (nodes.empty()) {
        return ZDD::empty(mgr);
    }

    // Sort by id (descending for bottom-up construction)
    // Use explicit comparison for C++11 compatibility
    struct TupleCompare {
        bool operator()(const std::tuple<int, int, int>& a,
                        const std::tuple<int, int, int>& b) const {
            return std::get<0>(a) > std::get<0>(b);
        }
    };
    std::sort(nodes.begin(), nodes.end(), TupleCompare());

    // Build ZDD bottom-up
    std::unordered_map<int, Arc> arc_map;
    arc_map[0] = ARC_TERMINAL_0;
    arc_map[-1] = ARC_TERMINAL_1;

    // Calculate variable for each node (based on position)
    int current_level = (root_level > 0) ? root_level : static_cast<int>(nodes.size());

    for (size_t i = 0; i < nodes.size(); ++i) {
        int id = std::get<0>(nodes[i]);
        int lo = std::get<1>(nodes[i]);
        int hi = std::get<2>(nodes[i]);

        Arc lo_arc = arc_map.count(lo) ? arc_map[lo] : ARC_TERMINAL_0;
        Arc hi_arc = arc_map.count(hi) ? arc_map[hi] : ARC_TERMINAL_0;

        bddvar var = static_cast<bddvar>(id);  // Use node id as variable
        Arc arc = mgr.get_or_create_node_zdd(var, lo_arc, hi_arc, true);
        arc_map[id] = arc;
        current_level--;
    }

    // Root is the node with smallest id (or first in sorted list)
    int root_id = std::get<0>(nodes.back());
    return ZDD(&mgr, arc_map[root_id]);
}

ZDD import_zdd_as_graphillion(DDManager& mgr, const std::string& filename, int root_level) {
    std::ifstream ifs(filename);
    if (!ifs) return ZDD::empty(mgr);
    return import_zdd_as_graphillion(mgr, ifs, root_level);
}

void export_zdd_as_graphillion(const ZDD& zdd, std::ostream& os, int root_level) {
    if (!zdd.manager()) return;

    DDManager* mgr = zdd.manager();

    if (zdd.is_terminal()) {
        if (zdd.is_one()) {
            os << "1 0 -1\n";  // Single node pointing to terminals
        }
        // Empty ZDD outputs nothing
        return;
    }

    // Collect nodes
    std::unordered_set<bddindex> visited;
    std::vector<bddindex> nodes;
    std::stack<Arc> stack;
    stack.push(zdd.arc());

    while (!stack.empty()) {
        Arc a = stack.top();
        stack.pop();

        if (a.is_constant()) continue;

        bddindex idx = a.index();
        if (visited.count(idx)) continue;
        visited.insert(idx);
        nodes.push_back(idx);

        const DDNode& node = mgr->node_at(idx);
        stack.push(node.arc0());
        stack.push(node.arc1());
    }

    // Sort by variable (descending)
    std::sort(nodes.begin(), nodes.end(), [mgr](bddindex a, bddindex b) {
        return mgr->node_at(a).var() < mgr->node_at(b).var();
    });

    // Create id mapping (1-indexed)
    std::unordered_map<bddindex, int> id_map;
    for (size_t i = 0; i < nodes.size(); ++i) {
        id_map[nodes[i]] = static_cast<int>(i + 1);
    }

    // Helper to convert arc to graphillion id
    auto arc_to_id = [&](Arc a) -> int {
        if (a.is_constant()) {
            return a.terminal_value() ? -1 : 0;
        }
        return id_map[a.index()];
    };

    // Output header
    os << "B\n";

    // Output nodes
    for (bddindex idx : nodes) {
        const DDNode& node = mgr->node_at(idx);
        os << id_map[idx] << " "
           << arc_to_id(node.arc0()) << " "
           << arc_to_id(node.arc1()) << "\n";
    }
}

void export_zdd_as_graphillion(const ZDD& zdd, const std::string& filename, int root_level) {
    std::ofstream ofs(filename);
    if (!ofs) return;
    export_zdd_as_graphillion(zdd, ofs, root_level);
}

// ============== Knuth Format ==============
// Format similar to Knuth's TAOCP BDD format

ZDD import_zdd_as_knuth(DDManager& mgr, std::istream& is, bool is_hex, int root_level) {
    std::string line;
    std::vector<std::tuple<bddvar, int, int>> nodes;  // (var, lo, hi)
    int node_count = 0;

    while (std::getline(is, line)) {
        if (line.empty() || line[0] == '#') continue;

        std::istringstream iss(line);
        int var, lo, hi;

        if (is_hex) {
            iss >> std::hex >> var >> lo >> hi;
        } else {
            iss >> var >> lo >> hi;
        }

        if (iss) {
            nodes.push_back(std::make_tuple(static_cast<bddvar>(var), lo, hi));
            node_count++;
        }
    }

    if (nodes.empty()) {
        return ZDD::empty(mgr);
    }

    // Build ZDD (nodes are listed in order, referenced by index)
    std::vector<Arc> arc_map(node_count + 2);
    arc_map[0] = ARC_TERMINAL_0;
    arc_map[1] = ARC_TERMINAL_1;

    for (size_t i = 0; i < nodes.size(); ++i) {
        bddvar var = std::get<0>(nodes[i]);
        int lo_idx = std::get<1>(nodes[i]);
        int hi_idx = std::get<2>(nodes[i]);

        Arc lo_arc = (lo_idx >= 0 && lo_idx < static_cast<int>(arc_map.size())) ?
                     arc_map[lo_idx] : ARC_TERMINAL_0;
        Arc hi_arc = (hi_idx >= 0 && hi_idx < static_cast<int>(arc_map.size())) ?
                     arc_map[hi_idx] : ARC_TERMINAL_0;

        Arc arc = mgr.get_or_create_node_zdd(var, lo_arc, hi_arc, true);
        arc_map[i + 2] = arc;
    }

    // Root is the last node
    return ZDD(&mgr, arc_map[node_count + 1]);
}

ZDD import_zdd_as_knuth(DDManager& mgr, const std::string& filename, bool is_hex, int root_level) {
    std::ifstream ifs(filename);
    if (!ifs) return ZDD::empty(mgr);
    return import_zdd_as_knuth(mgr, ifs, is_hex, root_level);
}

void export_zdd_as_knuth(const ZDD& zdd, std::ostream& os, bool is_hex) {
    if (!zdd.manager()) return;

    DDManager* mgr = zdd.manager();

    if (zdd.is_terminal()) {
        // Output comment for terminal
        os << "# " << (zdd.is_one() ? "base" : "empty") << "\n";
        return;
    }

    // Collect nodes
    std::unordered_set<bddindex> visited;
    std::vector<bddindex> nodes;
    std::stack<Arc> stack;
    stack.push(zdd.arc());

    while (!stack.empty()) {
        Arc a = stack.top();
        stack.pop();

        if (a.is_constant()) continue;

        bddindex idx = a.index();
        if (visited.count(idx)) continue;
        visited.insert(idx);
        nodes.push_back(idx);

        const DDNode& node = mgr->node_at(idx);
        stack.push(node.arc0());
        stack.push(node.arc1());
    }

    // Sort by variable (ascending, for bottom-up output)
    std::sort(nodes.begin(), nodes.end(), [mgr](bddindex a, bddindex b) {
        return mgr->node_at(a).var() > mgr->node_at(b).var();
    });

    // Create index mapping (2-indexed, 0=terminal0, 1=terminal1)
    std::unordered_map<bddindex, int> idx_map;
    for (size_t i = 0; i < nodes.size(); ++i) {
        idx_map[nodes[i]] = static_cast<int>(i + 2);
    }

    auto arc_to_idx = [&](Arc a) -> int {
        if (a.is_constant()) {
            return a.terminal_value() ? 1 : 0;
        }
        return idx_map[a.index()];
    };

    // Output header comment
    os << "# ZDD Knuth format\n";
    os << "# node_count=" << nodes.size() << "\n";

    // Output nodes
    for (bddindex idx : nodes) {
        const DDNode& node = mgr->node_at(idx);
        if (is_hex) {
            os << std::hex << node.var() << " "
               << arc_to_idx(node.arc0()) << " "
               << arc_to_idx(node.arc1()) << std::dec << "\n";
        } else {
            os << node.var() << " "
               << arc_to_idx(node.arc0()) << " "
               << arc_to_idx(node.arc1()) << "\n";
        }
    }
}

void export_zdd_as_knuth(const ZDD& zdd, const std::string& filename, bool is_hex) {
    std::ofstream ofs(filename);
    if (!ofs) return;
    export_zdd_as_knuth(zdd, ofs, is_hex);
}

// ============== lib_bdd Format ==============
// lib-bdd binary format: 10 bytes per node (little-endian)
// - uint16_t level (0xFFFF for terminal)
// - uint32_t low pointer
// - uint32_t high pointer
// Index 0 = false terminal, Index 1 = true terminal

namespace {
    // lib_bdd constants
    constexpr std::uint16_t LIBBDD_TERMINAL_LEVEL = 0xFFFF;
    constexpr std::uint32_t LIBBDD_FALSE_PTR = 0;
    constexpr std::uint32_t LIBBDD_TRUE_PTR = 1;
    constexpr std::size_t LIBBDD_NODE_SIZE = 10;

    // Write little-endian uint16_t
    void write_le_u16(std::ostream& os, std::uint16_t value) {
        char bytes[2];
        bytes[0] = static_cast<char>(value & 0xFF);
        bytes[1] = static_cast<char>((value >> 8) & 0xFF);
        os.write(bytes, 2);
    }

    // Write little-endian uint32_t
    void write_le_u32(std::ostream& os, std::uint32_t value) {
        char bytes[4];
        bytes[0] = static_cast<char>(value & 0xFF);
        bytes[1] = static_cast<char>((value >> 8) & 0xFF);
        bytes[2] = static_cast<char>((value >> 16) & 0xFF);
        bytes[3] = static_cast<char>((value >> 24) & 0xFF);
        os.write(bytes, 4);
    }

    // Read little-endian uint16_t
    std::uint16_t read_le_u16(std::istream& is) {
        unsigned char bytes[2];
        is.read(reinterpret_cast<char*>(bytes), 2);
        return static_cast<std::uint16_t>(bytes[0]) |
               (static_cast<std::uint16_t>(bytes[1]) << 8);
    }

    // Read little-endian uint32_t
    std::uint32_t read_le_u32(std::istream& is) {
        unsigned char bytes[4];
        is.read(reinterpret_cast<char*>(bytes), 4);
        return static_cast<std::uint32_t>(bytes[0]) |
               (static_cast<std::uint32_t>(bytes[1]) << 8) |
               (static_cast<std::uint32_t>(bytes[2]) << 16) |
               (static_cast<std::uint32_t>(bytes[3]) << 24);
    }

    // lib_bdd node structure for reading
    struct LibBddNode {
        std::uint16_t level;
        std::uint32_t low;
        std::uint32_t high;

        bool is_terminal() const { return level == LIBBDD_TERMINAL_LEVEL; }
        bool is_false() const { return is_terminal() && low == LIBBDD_FALSE_PTR; }
        bool is_true() const { return is_terminal() && low == LIBBDD_TRUE_PTR; }
    };

    // Read a single lib_bdd node
    bool read_libbdd_node(std::istream& is, LibBddNode& node) {
        node.level = read_le_u16(is);
        node.low = read_le_u32(is);
        node.high = read_le_u32(is);
        return is.good() || is.eof();
    }

    // Write a single lib_bdd node
    void write_libbdd_node(std::ostream& os, std::uint16_t level,
                           std::uint32_t low, std::uint32_t high) {
        write_le_u16(os, level);
        write_le_u32(os, low);
        write_le_u32(os, high);
    }
}

// Import BDD from lib_bdd format
BDD import_bdd_as_libbdd(DDManager& mgr, std::istream& is) {
    std::vector<LibBddNode> nodes;

    // Read all nodes
    while (true) {
        LibBddNode node;
        std::streampos pos = is.tellg();
        node.level = read_le_u16(is);

        if (is.eof()) break;
        if (!is.good()) break;

        node.low = read_le_u32(is);
        node.high = read_le_u32(is);

        if (!is.good() && !is.eof()) break;

        nodes.push_back(node);

        if (is.eof()) break;
    }

    if (nodes.empty()) {
        return mgr.bdd_zero();
    }

    // Only terminals
    if (nodes.size() <= 2) {
        if (nodes.size() == 1) {
            return nodes[0].is_true() ? mgr.bdd_one() : mgr.bdd_zero();
        }
        // Two nodes: usually false and true terminals
        return mgr.bdd_one();
    }

    // Build mapping from lib_bdd index to Arc
    std::vector<Arc> arc_map(nodes.size());
    arc_map[LIBBDD_FALSE_PTR] = ARC_TERMINAL_0;
    arc_map[LIBBDD_TRUE_PTR] = ARC_TERMINAL_1;

    // Process nodes bottom-up (higher indices first, which are typically deeper)
    // Actually, lib_bdd stores nodes in bottom-up order, so we process in order
    for (std::size_t i = 2; i < nodes.size(); ++i) {
        const LibBddNode& n = nodes[i];

        if (n.is_terminal()) {
            // Additional terminal - shouldn't happen in well-formed BDD
            arc_map[i] = n.is_true() ? ARC_TERMINAL_1 : ARC_TERMINAL_0;
            continue;
        }

        // Validate indices
        if (n.low >= i || n.high >= i) {
            // Forward reference - invalid format
            return mgr.bdd_zero();
        }

        Arc lo_arc = arc_map[n.low];
        Arc hi_arc = arc_map[n.high];

        // lib_bdd level is the variable number (1-indexed in SAPPOROBDD2)
        bddvar var = static_cast<bddvar>(n.level + 1);

        // Ensure variable exists
        while (mgr.var_count() < var) {
            mgr.new_var();
        }

        Arc arc = mgr.get_or_create_node_bdd(var, lo_arc, hi_arc, true);
        arc_map[i] = arc;
    }

    // Root is the last node
    return BDD(&mgr, arc_map[nodes.size() - 1]);
}

BDD import_bdd_as_libbdd(DDManager& mgr, const std::string& filename) {
    std::ifstream ifs(filename, std::ios::binary);
    if (!ifs) return mgr.bdd_zero();
    return import_bdd_as_libbdd(mgr, ifs);
}

// Import ZDD from lib_bdd format
ZDD import_zdd_as_libbdd(DDManager& mgr, std::istream& is) {
    std::vector<LibBddNode> nodes;

    // Read all nodes
    while (true) {
        LibBddNode node;
        node.level = read_le_u16(is);

        if (is.eof()) break;
        if (!is.good()) break;

        node.low = read_le_u32(is);
        node.high = read_le_u32(is);

        if (!is.good() && !is.eof()) break;

        nodes.push_back(node);

        if (is.eof()) break;
    }

    if (nodes.empty()) {
        return ZDD::empty(mgr);
    }

    // Only terminals
    if (nodes.size() <= 2) {
        if (nodes.size() == 1) {
            return nodes[0].is_true() ? ZDD::base(mgr) : ZDD::empty(mgr);
        }
        return ZDD::base(mgr);
    }

    // Build mapping from lib_bdd index to Arc
    std::vector<Arc> arc_map(nodes.size());
    arc_map[LIBBDD_FALSE_PTR] = ARC_TERMINAL_0;
    arc_map[LIBBDD_TRUE_PTR] = ARC_TERMINAL_1;

    // Process nodes in order (lib_bdd stores bottom-up)
    for (std::size_t i = 2; i < nodes.size(); ++i) {
        const LibBddNode& n = nodes[i];

        if (n.is_terminal()) {
            arc_map[i] = n.is_true() ? ARC_TERMINAL_1 : ARC_TERMINAL_0;
            continue;
        }

        if (n.low >= i || n.high >= i) {
            return ZDD::empty(mgr);
        }

        Arc lo_arc = arc_map[n.low];
        Arc hi_arc = arc_map[n.high];

        bddvar var = static_cast<bddvar>(n.level + 1);

        while (mgr.var_count() < var) {
            mgr.new_var();
        }

        Arc arc = mgr.get_or_create_node_zdd(var, lo_arc, hi_arc, true);
        arc_map[i] = arc;
    }

    return ZDD(&mgr, arc_map[nodes.size() - 1]);
}

ZDD import_zdd_as_libbdd(DDManager& mgr, const std::string& filename) {
    std::ifstream ifs(filename, std::ios::binary);
    if (!ifs) return ZDD::empty(mgr);
    return import_zdd_as_libbdd(mgr, ifs);
}

// Export BDD to lib_bdd format
void export_bdd_as_libbdd(const BDD& bdd, std::ostream& os) {
    if (!bdd.manager()) return;

    DDManager* mgr = bdd.manager();

    // Write false terminal (index 0)
    write_libbdd_node(os, LIBBDD_TERMINAL_LEVEL, LIBBDD_FALSE_PTR, LIBBDD_FALSE_PTR);

    // Write true terminal (index 1)
    write_libbdd_node(os, LIBBDD_TERMINAL_LEVEL, LIBBDD_TRUE_PTR, LIBBDD_TRUE_PTR);

    if (bdd.is_terminal()) {
        // For terminal BDDs, we only need the two terminal nodes
        // The BDD itself is represented by which terminal it equals
        return;
    }

    // Collect all internal nodes
    std::unordered_set<bddindex> visited;
    std::vector<bddindex> nodes;
    std::stack<Arc> stack;
    stack.push(bdd.arc());

    while (!stack.empty()) {
        Arc a = stack.top();
        stack.pop();

        if (a.is_constant()) continue;

        bddindex idx = a.index();
        if (visited.count(idx)) continue;
        visited.insert(idx);
        nodes.push_back(idx);

        const DDNode& node = mgr->node_at(idx);
        stack.push(node.arc0());
        stack.push(node.arc1());
    }

    // Sort by variable (descending) for bottom-up order in lib_bdd
    std::sort(nodes.begin(), nodes.end(), [mgr](bddindex a, bddindex b) {
        return mgr->node_at(a).var() > mgr->node_at(b).var();
    });

    // Create index mapping: SAPPOROBDD index -> lib_bdd index
    // Indices 0 and 1 are reserved for terminals
    std::unordered_map<bddindex, std::uint32_t> idx_map;
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        idx_map[nodes[i]] = static_cast<std::uint32_t>(i + 2);
    }

    // Helper to convert Arc to lib_bdd pointer
    auto arc_to_ptr = [&](Arc a) -> std::uint32_t {
        if (a.is_constant()) {
            bool value = a.terminal_value();
            // Handle negation edge for BDD
            if (a.is_negated()) value = !value;
            return value ? LIBBDD_TRUE_PTR : LIBBDD_FALSE_PTR;
        }
        std::uint32_t ptr = idx_map[a.index()];
        // Note: lib_bdd doesn't support negation edges directly
        // For BDDs with negation edges, the structure needs special handling
        return ptr;
    };

    // Write internal nodes
    for (bddindex idx : nodes) {
        const DDNode& node = mgr->node_at(idx);

        // lib_bdd level is 0-indexed
        std::uint16_t level = static_cast<std::uint16_t>(node.var() - 1);

        Arc arc0 = node.arc0();
        Arc arc1 = node.arc1();

        // Handle negated root if necessary
        bool root_neg = bdd.arc().is_negated() && idx == bdd.arc().index();

        std::uint32_t low = arc_to_ptr(arc0);
        std::uint32_t high = arc_to_ptr(arc1);

        write_libbdd_node(os, level, low, high);
    }
}

void export_bdd_as_libbdd(const BDD& bdd, const std::string& filename) {
    std::ofstream ofs(filename, std::ios::binary);
    if (!ofs) return;
    export_bdd_as_libbdd(bdd, ofs);
}

// Export ZDD to lib_bdd format
void export_zdd_as_libbdd(const ZDD& zdd, std::ostream& os) {
    if (!zdd.manager()) return;

    DDManager* mgr = zdd.manager();

    // Write false terminal (index 0)
    write_libbdd_node(os, LIBBDD_TERMINAL_LEVEL, LIBBDD_FALSE_PTR, LIBBDD_FALSE_PTR);

    // Write true terminal (index 1)
    write_libbdd_node(os, LIBBDD_TERMINAL_LEVEL, LIBBDD_TRUE_PTR, LIBBDD_TRUE_PTR);

    if (zdd.is_terminal()) {
        return;
    }

    // Collect all internal nodes
    std::unordered_set<bddindex> visited;
    std::vector<bddindex> nodes;
    std::stack<Arc> stack;
    stack.push(zdd.arc());

    while (!stack.empty()) {
        Arc a = stack.top();
        stack.pop();

        if (a.is_constant()) continue;

        bddindex idx = a.index();
        if (visited.count(idx)) continue;
        visited.insert(idx);
        nodes.push_back(idx);

        const DDNode& node = mgr->node_at(idx);
        stack.push(node.arc0());
        stack.push(node.arc1());
    }

    // Sort by variable (descending) for bottom-up order
    std::sort(nodes.begin(), nodes.end(), [mgr](bddindex a, bddindex b) {
        return mgr->node_at(a).var() > mgr->node_at(b).var();
    });

    // Create index mapping
    std::unordered_map<bddindex, std::uint32_t> idx_map;
    for (std::size_t i = 0; i < nodes.size(); ++i) {
        idx_map[nodes[i]] = static_cast<std::uint32_t>(i + 2);
    }

    auto arc_to_ptr = [&](Arc a) -> std::uint32_t {
        if (a.is_constant()) {
            return a.terminal_value() ? LIBBDD_TRUE_PTR : LIBBDD_FALSE_PTR;
        }
        return idx_map[a.index()];
    };

    // Write internal nodes
    for (bddindex idx : nodes) {
        const DDNode& node = mgr->node_at(idx);

        std::uint16_t level = static_cast<std::uint16_t>(node.var() - 1);
        std::uint32_t low = arc_to_ptr(node.arc0());
        std::uint32_t high = arc_to_ptr(node.arc1());

        write_libbdd_node(os, level, low, high);
    }
}

void export_zdd_as_libbdd(const ZDD& zdd, const std::string& filename) {
    std::ofstream ofs(filename, std::ios::binary);
    if (!ofs) return;
    export_zdd_as_libbdd(zdd, ofs);
}

// ============== SVG Format ==============

void export_zdd_as_svg(const ZDD& zdd, std::ostream& os, const SvgExportOptions& options) {
    if (!zdd.manager()) return;

    DDManager* mgr = zdd.manager();

    // Collect nodes and organize by level
    std::unordered_set<bddindex> visited;
    std::map<bddvar, std::vector<bddindex>> levels;  // var -> nodes at that level
    std::stack<Arc> stack;

    if (!zdd.is_terminal()) {
        stack.push(zdd.arc());
    }

    while (!stack.empty()) {
        Arc a = stack.top();
        stack.pop();

        if (a.is_constant()) continue;

        bddindex idx = a.index();
        if (visited.count(idx)) continue;
        visited.insert(idx);

        const DDNode& node = mgr->node_at(idx);
        levels[node.var()].push_back(idx);

        stack.push(node.arc0());
        stack.push(node.arc1());
    }

    // Calculate positions
    std::unordered_map<bddindex, std::pair<int, int>> positions;
    int current_y = options.node_radius + 20;
    int level_idx = 0;

    // Sort variables
    std::vector<bddvar> sorted_vars;
    for (const auto& kv : levels) {
        sorted_vars.push_back(kv.first);
    }
    std::sort(sorted_vars.begin(), sorted_vars.end());

    // Assign positions
    for (bddvar var : sorted_vars) {
        const auto& level_nodes = levels[var];
        int level_width = static_cast<int>(level_nodes.size()) * options.horizontal_gap;
        int start_x = (options.width - level_width) / 2;

        for (size_t i = 0; i < level_nodes.size(); ++i) {
            positions[level_nodes[i]] = {
                start_x + static_cast<int>(i) * options.horizontal_gap + options.horizontal_gap / 2,
                current_y
            };
        }
        current_y += options.level_gap;
        level_idx++;
    }

    // Terminal positions
    int terminal_y = current_y;
    int term0_x = options.width / 3;
    int term1_x = 2 * options.width / 3;

    // Start SVG
    os << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    os << "<svg xmlns=\"http://www.w3.org/2000/svg\" "
       << "width=\"" << options.width << "\" "
       << "height=\"" << std::max(options.height, terminal_y + 50) << "\">\n";

    // Style definitions
    os << "  <defs>\n";
    os << "    <marker id=\"arrowhead\" markerWidth=\"10\" markerHeight=\"7\" "
       << "refX=\"9\" refY=\"3.5\" orient=\"auto\">\n";
    os << "      <polygon points=\"0 0, 10 3.5, 0 7\" fill=\"" << options.edge_1_color << "\"/>\n";
    os << "    </marker>\n";
    os << "  </defs>\n";

    // Background
    os << "  <rect width=\"100%\" height=\"100%\" fill=\"white\"/>\n";

    // Draw edges first (so nodes are on top)
    for (std::unordered_map<bddindex, std::pair<int, int> >::const_iterator it = positions.begin();
         it != positions.end(); ++it) {
        bddindex idx = it->first;
        int x = it->second.first;
        int y = it->second.second;
        const DDNode& node = mgr->node_at(idx);

        // Low edge (dashed)
        if (node.arc0().is_constant()) {
            int target_x = node.arc0().terminal_value() ? term1_x : term0_x;
            os << "  <line x1=\"" << x << "\" y1=\"" << (y + options.node_radius)
               << "\" x2=\"" << target_x << "\" y2=\"" << terminal_y
               << "\" stroke=\"" << options.edge_0_color << "\" stroke-dasharray=\"5,5\"/>\n";
        } else {
            std::pair<int, int> target = positions.at(node.arc0().index());
            int tx = target.first, ty = target.second;
            os << "  <line x1=\"" << x << "\" y1=\"" << (y + options.node_radius)
               << "\" x2=\"" << tx << "\" y2=\"" << (ty - options.node_radius)
               << "\" stroke=\"" << options.edge_0_color << "\" stroke-dasharray=\"5,5\"/>\n";
        }

        // High edge (solid)
        if (node.arc1().is_constant()) {
            int target_x = node.arc1().terminal_value() ? term1_x : term0_x;
            os << "  <line x1=\"" << x << "\" y1=\"" << (y + options.node_radius)
               << "\" x2=\"" << target_x << "\" y2=\"" << terminal_y
               << "\" stroke=\"" << options.edge_1_color << "\"/>\n";
        } else {
            std::pair<int, int> target = positions.at(node.arc1().index());
            int tx = target.first, ty = target.second;
            os << "  <line x1=\"" << x << "\" y1=\"" << (y + options.node_radius)
               << "\" x2=\"" << tx << "\" y2=\"" << (ty - options.node_radius)
               << "\" stroke=\"" << options.edge_1_color << "\"/>\n";
        }
    }

    // Draw nodes
    for (std::unordered_map<bddindex, std::pair<int, int> >::const_iterator it = positions.begin();
         it != positions.end(); ++it) {
        bddindex idx = it->first;
        int x = it->second.first;
        int y = it->second.second;
        const DDNode& node = mgr->node_at(idx);

        os << "  <circle cx=\"" << x << "\" cy=\"" << y
           << "\" r=\"" << options.node_radius
           << "\" fill=\"" << options.node_fill_color
           << "\" stroke=\"" << options.node_stroke_color << "\"/>\n";

        if (options.show_variable_labels) {
            os << "  <text x=\"" << x << "\" y=\"" << (y + options.font_size / 3)
               << "\" text-anchor=\"middle\" font-family=\"" << options.font_family
               << "\" font-size=\"" << options.font_size << "\">"
               << "x" << node.var() << "</text>\n";
        }
    }

    // Draw terminals
    if (options.show_terminal_labels || !zdd.is_terminal()) {
        // Terminal 0
        os << "  <rect x=\"" << (term0_x - 15) << "\" y=\"" << (terminal_y - 10)
           << "\" width=\"30\" height=\"20\" fill=\"" << options.terminal_0_color
           << "\" stroke=\"" << options.node_stroke_color << "\"/>\n";
        os << "  <text x=\"" << term0_x << "\" y=\"" << (terminal_y + 5)
           << "\" text-anchor=\"middle\" font-family=\"" << options.font_family
           << "\" font-size=\"" << options.font_size << "\">0</text>\n";

        // Terminal 1
        os << "  <rect x=\"" << (term1_x - 15) << "\" y=\"" << (terminal_y - 10)
           << "\" width=\"30\" height=\"20\" fill=\"" << options.terminal_1_color
           << "\" stroke=\"" << options.node_stroke_color << "\"/>\n";
        os << "  <text x=\"" << term1_x << "\" y=\"" << (terminal_y + 5)
           << "\" text-anchor=\"middle\" font-family=\"" << options.font_family
           << "\" font-size=\"" << options.font_size << "\">1</text>\n";
    }

    // Draw root indicator
    if (!zdd.is_terminal() && !positions.empty()) {
        std::pair<int, int> root_pos = positions.at(zdd.arc().index());
        int rx = root_pos.first, ry = root_pos.second;
        os << "  <line x1=\"" << rx << "\" y1=\"10\" x2=\"" << rx
           << "\" y2=\"" << (ry - options.node_radius)
           << "\" stroke=\"" << options.node_stroke_color
           << "\" marker-end=\"url(#arrowhead)\"/>\n";
    }

    // Legend
    os << "  <text x=\"10\" y=\"" << (terminal_y + 40)
       << "\" font-family=\"" << options.font_family
       << "\" font-size=\"10\">Dashed: 0-edge, Solid: 1-edge</text>\n";

    os << "</svg>\n";
}

void export_zdd_as_svg(const ZDD& zdd, const std::string& filename,
                       const SvgExportOptions& options) {
    std::ofstream ofs(filename);
    if (!ofs) return;
    export_zdd_as_svg(zdd, ofs, options);
}

} // namespace sbdd2
