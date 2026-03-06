#pragma once

#include "core/netlist.hpp"

// Remove nets that are unconnected.
void prune_unconnected_nets(RawNetlist &netlist);

void try_assign(std::unique_ptr<RawNetlist> &raw);
