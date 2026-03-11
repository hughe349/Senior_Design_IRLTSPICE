#pragma once

#include "core/compiler.hpp"
#include "core/netlist.hpp"
#include <variant>
#include <vector>

typedef enum circuit_input_t { CIRCUIT_INPUT } circuit_input_t;
typedef enum ground_input_t { GROUND_INPUT } ground_input_t;
typedef std::variant<circuit_input_t, ground_input_t, uint32_t> CellInputId;

typedef struct _StdCell {
    // This cell's arbitrary id. Must be the same as index in AssignedNetList->cells vec
    uint32_t id;
    // This cell's output buffer
    RawVert buffer;
    // Nets in this cell including inputs and output
    std::vector<RawVert> nets;
    // Components in this cell. Not counting output buffer
    std::vector<RawVert> components;
    // The net right before the output buffer. Element of nets field
    RawVert pre_buffer_output_net;
    // input nets and which std cell the come from. Subset of nets field
    std::vector<std::pair<RawVert, CellInputId>> input_nets;

    void to_str(std::ostream &ostream, RawNetlist const raw);
} StdCell;

// An AssignedNetList has assigned each component (not each net) to std cells
// The raw_list should not be modified, as this struct may hold references to it.
typedef struct _AssignedNetList {
    std::unique_ptr<const RawNetlist> raw_list;
    std::vector<StdCell> cells;

    // Maps a vert for a cell's buffer to a cell reference.
    StdCell const &get_cell(RawVert v);
} AssignedNetlist;

class TspiceRouter {
  private:
    IrlCompiler const &compiler;

  public:
    TspiceRouter(IrlCompiler const &compiler) : compiler(compiler) {};

    // Remove nets that are unconnected.
    void prune_unconnected_nets(RawNetlist &netlist);

    // Either returns null, and raw remains a valid pointer, or returns a real pointer and raw is
    // invalidated (moved to the returned AssignedNetlist).
    std::unique_ptr<AssignedNetlist> try_assign(std::unique_ptr<RawNetlist> &raw);
};
