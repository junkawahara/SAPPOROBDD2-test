// SAPPOROBDD 2.0 - Import/Export implementation
// MIT License

#include "sbdd2/io.hpp"
#include <cstring>
#include <sstream>
#include <unordered_map>
#include <unordered_set>
#include <stack>
#include <algorithm>

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

} // namespace sbdd2
