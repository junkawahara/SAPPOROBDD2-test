// SAPPOROBDD 2.0 - Main header file
// MIT License

#ifndef SBDD2_HPP
#define SBDD2_HPP

// Core headers
#include "types.hpp"
#include "exception.hpp"
#include "dd_node.hpp"
#include "dd_manager.hpp"
#include "dd_node_ref.hpp"
#include "dd_base.hpp"
#include "bdd.hpp"
#include "zdd.hpp"

// Extended DD types
#include "unreduced_bdd.hpp"
#include "unreduced_zdd.hpp"
#include "qdd.hpp"

// Multi-Terminal DD types
#include "mtdd_base.hpp"
#include "mtbdd.hpp"
#include "mtzdd.hpp"

// Multi-Valued DD types
#include "mvdd_base.hpp"
#include "mvzdd.hpp"
#include "mvbdd.hpp"

// Derived classes
#include "pidd.hpp"
#include "seqbdd.hpp"
#include "gbase.hpp"
#include "bddct.hpp"

// Helper functions
#include "zdd_helper.hpp"

// I/O
#include "io.hpp"

namespace sbdd2 {

// Library version
constexpr int VERSION_MAJOR = 2;
constexpr int VERSION_MINOR = 0;
constexpr int VERSION_PATCH = 0;

// Version string
inline const char* version() {
    return "2.0.0";
}

} // namespace sbdd2

#endif // SBDD2_HPP
