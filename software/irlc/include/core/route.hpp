#pragma once

#include "core/netlist.hpp"
#include <variant>
#include <vector>

typedef enum circuit_input_t { CIRCUIT_INPUT } circuit_input_t;
typedef std::variant<circuit_input_t, uint32_t> CellInputId;

typedef struct _StdCell {
    uint32_t id;
    std::vector<RawVert> nets;
    std::vector<RawVert> components;
    // The net right before the output buffer. Element of nets field
    RawVert output_net;
    // input nets and which std cell the come from. Subset of nets field
    std::vector<std::pair<RawVert, CellInputId>> input_nets;
} StdCell;

// An AssignedNetList has assigned each component (not each net) to std cells
// The raw_list should not be modified, as this struct may hold references to it.
typedef struct _AssignedNetList {
    std::unique_ptr<const RawNetlist> raw_list;
    std::vector<StdCell> cells;
} AssignedNetlist;

// Remove nets that are unconnected.
void prune_unconnected_nets(RawNetlist &netlist);

// Either returns null, and raw remains a valid pointer, or returns a real pointer and raw is
// invalidated (moved to the returned AssignedNetlist).
std::unique_ptr<AssignedNetlist> try_assign(std::unique_ptr<RawNetlist> &raw);
