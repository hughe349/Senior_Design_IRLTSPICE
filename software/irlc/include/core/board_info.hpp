#pragma once
#include "core/netlist.hpp"
#include <cstdint>
#include <ranges>
#include <variant>
#include <vector>

typedef std::variant<struct FloatingCon, struct SpecialNetCon, struct RoutableColCon,
                     struct ComponentColConn>
    ColCon;
typedef std::variant<struct FloatingCon, struct SpecialNetCon, struct RoutableRowCon,
                     struct ChainedRowCon, struct BufferOutputRowCon, struct BufferInputRowCon>
    RowCon;

// A physical crossbar on the board and what it is connected to.
// By convention "rows" are used to connect "cols" together.
// So in a std cell rows are the nets and cols are the components
typedef class PhysCrossbar {
  public:
    // This crossbar's id as recognized by the micro
    uint8_t id;
    std::vector<RowCon> rows;
    std::vector<ColCon> cols;
} PhysCrossbar;

// One or more crossbars whose rows are connected together.
// Improperly setting up the chaining leads to undefined behavior
typedef class ChainedCrossbar {
  public:
    template <typename... PCross> inline ChainedCrossbar(PCross... nbars);

    std::vector<PhysCrossbar> bars;
} ChainedCrossbar;

// A physical standard cell, with bar and buffer
// Should be onle one crossbar row that has a BufferRowCon.
typedef struct PhysStdCell {
    // Id of this cell. Should match id of buffer in the cell as referenced by the row con
    uint8_t id;
    ChainedCrossbar crossbars;
} PhysStdCell;

// Every std cell is the same. All are routable to each other via a single parent chained crossbar
typedef struct SimpleTspiceInfo {
    ChainedCrossbar root;
    std::vector<PhysStdCell> cells;
} SimpleTspiceInfo;

// CONNECTION KINDS

// A row/col that is literally just connected to nothing. Good for routing within a cell
typedef struct FloatingCon {
} FloatingCon;

// A row/col that is hard-wired to either a voltage source or circuit input/output
// The kind is not allowed to be WIRE
typedef struct SpecialNetCon {
    NetKind kind;
} SpecialNetCon;

// A simple connection from a crossbar to a component
typedef struct ComponentColConn {
    // The kind of component this column goes to
    NetlistVertexKind kind;
    // What pin kind of the component this column goes to
    NetlistEdgeKind pin_kind;
    // Id of the component this column goes to.
    // For programmable components (resistors) this must be what the micro recognizes, for everythin
    // else this just must be unique.
    int32_t id;
} ComponentColConn;

// A connection from this child crossbar to a parent crossbar, allowing routing
typedef struct RoutableRowCon {
    uint8_t parent_id;
    // which col of the parent this row is connected to
    uint8_t parent_col;
} RoutableRowCon;

// A connection from this parent crossbar to child parent crossbar, allowing routing
typedef struct RoutableColCon {
    uint8_t child_id;
    // which row of the child this col is connected to
    uint8_t child_row;
} RoutableColCon;

// A row that is hard wired to a buffer input
typedef struct BufferInputRowCon {
} BufferInputRowCon;

// A row that is hard wired to a std cell's buffer output
typedef struct BufferOutputRowCon {
    uint32_t id;
} BufferOutputRowCon;

// A row that is chained to the row of another crossbar
typedef struct ChainedRowCon {
    // Id of parent this row is (potentially) connected to
    RowCon const &sibiling;
} ChainedRowCon;

// Board defs

const SimpleTspiceInfo MAIN_BOARD = []() {
    uint8_t i = -1;
    auto uniquie = [&i]() { return i--; };
    auto prev = [&i]() { return i; };

    const auto ROOT_BAR = 1;
    const auto CELL_0_BAR_0 = 2;
    const auto CELL_0_BAR_1 = 3;
    const auto CELL_1_BAR_0 = 3;
    const auto CELL_1_BAR_1 = 4;
    const auto CELL_2_BAR_0 = 4;
    const auto CELL_2_BAR_1 = 5;
    const auto CELL_3_BAR_0 = 5;
    const auto CELL_3_BAR_1 = 6;

#define STD_CELL(CELL_ID, FIRST_R)                                                                 \
    PhysStdCell {                                                                                  \
        .id = CELL_ID,                                                                             \
        .crossbars = ChainedCrossbar(                                                              \
            PhysCrossbar{                                                                          \
                .id = CELL_##CELL_ID##_BAR_0,                                                      \
                .rows =                                                                            \
                    std::vector<RowCon>{                                                           \
                        RoutableRowCon{.parent_id = ROOT_BAR, .parent_col = 0},                    \
                        RoutableRowCon{.parent_id = ROOT_BAR, .parent_col = 1},                    \
                        RoutableRowCon{.parent_id = ROOT_BAR, .parent_col = 2},                    \
                        BufferInputRowCon{},                                                       \
                        FloatingCon{},                                                             \
                        SpecialNetCon{.kind = V_GND},                                              \
                        FloatingCon{},                                                             \
                        FloatingCon{},                                                             \
                    },                                                                             \
                .cols =                                                                            \
                    std::vector<ColCon>{                                                           \
                        ComponentColConn{.kind = C, .pin_kind = PIN_C, .id = uniquie()},           \
                        ComponentColConn{.kind = C, .pin_kind = PIN_C, .id = prev()},              \
                        ComponentColConn{.kind = C, .pin_kind = PIN_C, .id = uniquie()},           \
                        ComponentColConn{.kind = C, .pin_kind = PIN_C, .id = prev()},              \
                        ComponentColConn{.kind = C, .pin_kind = PIN_C, .id = uniquie()},           \
                        ComponentColConn{.kind = C, .pin_kind = PIN_C, .id = prev()},              \
                        ComponentColConn{.kind = DIODE, .pin_kind = PIN_D_K, .id = uniquie()},     \
                        ComponentColConn{.kind = DIODE, .pin_kind = PIN_D_A, .id = prev()},        \
                        ComponentColConn{.kind = C, .pin_kind = PIN_C, .id = uniquie()},           \
                        ComponentColConn{.kind = C, .pin_kind = PIN_C, .id = prev()},              \
                        ComponentColConn{.kind = R, .pin_kind = PIN_R, .id = FIRST_R + 5},         \
                        ComponentColConn{.kind = R, .pin_kind = PIN_R, .id = FIRST_R + 5},         \
                        ComponentColConn{.kind = DIODE, .pin_kind = PIN_D_K, .id = uniquie()},     \
                        ComponentColConn{.kind = DIODE, .pin_kind = PIN_D_A, .id = prev()},        \
                        ComponentColConn{                                                          \
                            .kind = CUSTOM, .pin_kind = PIN_CUSTOM_A, .id = uniquie()},            \
                        ComponentColConn{.kind = CUSTOM, .pin_kind = PIN_CUSTOM_B, .id = prev()},  \
                    },                                                                             \
            },                                                                                     \
            PhysCrossbar{                                                                          \
                .id = CELL_##CELL_ID##_BAR_1,                                                      \
                .rows = std::vector<RowCon>(8),                                                    \
                .cols =                                                                            \
                    std::vector<ColCon>{                                                           \
                        ComponentColConn{                                                          \
                            .kind = OPAMP, .pin_kind = PIN_OPAMP_OUT, .id = uniquie()},            \
                        ComponentColConn{.kind = OPAMP, .pin_kind = PIN_OPAMP_PLUS, .id = prev()}, \
                        ComponentColConn{                                                          \
                            .kind = OPAMP, .pin_kind = PIN_OPAMP_MINUS, .id = prev()},             \
                        ComponentColConn{                                                          \
                            .kind = OPAMP, .pin_kind = PIN_OPAMP_OUT, .id = uniquie()},            \
                        ComponentColConn{.kind = OPAMP, .pin_kind = PIN_OPAMP_PLUS, .id = prev()}, \
                        ComponentColConn{                                                          \
                            .kind = OPAMP, .pin_kind = PIN_OPAMP_MINUS, .id = prev()},             \
                        ComponentColConn{.kind = R, .pin_kind = PIN_R, .id = FIRST_R + 2},         \
                        ComponentColConn{.kind = R, .pin_kind = PIN_R, .id = FIRST_R + 2},         \
                        ComponentColConn{.kind = R, .pin_kind = PIN_R, .id = FIRST_R + 3},         \
                        ComponentColConn{.kind = R, .pin_kind = PIN_R, .id = FIRST_R + 3},         \
                        ComponentColConn{.kind = R, .pin_kind = PIN_R, .id = FIRST_R + 4},         \
                        ComponentColConn{.kind = R, .pin_kind = PIN_R, .id = FIRST_R + 4},         \
                        ComponentColConn{.kind = R, .pin_kind = PIN_R, .id = FIRST_R + 0},         \
                        ComponentColConn{.kind = R, .pin_kind = PIN_R, .id = FIRST_R + 0},         \
                        ComponentColConn{.kind = R, .pin_kind = PIN_R, .id = FIRST_R + 1},         \
                        ComponentColConn{.kind = R, .pin_kind = PIN_R, .id = FIRST_R + 1},         \
                    },                                                                             \
            })                                                                                     \
    }

    return SimpleTspiceInfo{
        .root = ChainedCrossbar(PhysCrossbar{
            .id = ROOT_BAR,
            .rows =
                std::vector<RowCon>{
                    SpecialNetCon{.kind = INPUT},
                    BufferOutputRowCon{.id = 0},
                    BufferOutputRowCon{.id = 1},
                    BufferOutputRowCon{.id = 2},
                    BufferOutputRowCon{.id = 3},
                    FloatingCon{},
                    FloatingCon{},
                    FloatingCon{},
                },
            .cols =
                std::vector<ColCon>{
                    RoutableColCon{.child_id = CELL_0_BAR_0, .child_row = 0},
                    RoutableColCon{.child_id = CELL_0_BAR_0, .child_row = 1},
                    RoutableColCon{.child_id = CELL_0_BAR_0, .child_row = 2},
                    RoutableColCon{.child_id = CELL_1_BAR_0, .child_row = 0},
                    RoutableColCon{.child_id = CELL_1_BAR_0, .child_row = 1},
                    RoutableColCon{.child_id = CELL_1_BAR_0, .child_row = 2},
                    SpecialNetCon{.kind = OUTPUT},
                    RoutableColCon{.child_id = CELL_2_BAR_0, .child_row = 0},
                    RoutableColCon{.child_id = CELL_2_BAR_0, .child_row = 1},
                    RoutableColCon{.child_id = CELL_2_BAR_0, .child_row = 2},
                    RoutableColCon{.child_id = CELL_3_BAR_0, .child_row = 0},
                    RoutableColCon{.child_id = CELL_3_BAR_0, .child_row = 1},
                    RoutableColCon{.child_id = CELL_3_BAR_0, .child_row = 2},
                    FloatingCon{},
                    FloatingCon{},
                    FloatingCon{},
                },
        }),
        .cells = std::vector<PhysStdCell>{STD_CELL(0, 100), STD_CELL(1, 200), STD_CELL(2, 300),
                                          STD_CELL(3, 400)},
    };
}();

template <typename... PCross> inline ChainedCrossbar::ChainedCrossbar(PCross... nbars) {
    this->bars = std::vector{nbars...};
    for (auto &bar : bars | std::views::drop(1)) {
        bar.rows.clear();
        for (int i = 0; i < bar.rows.size(); i++) {
            bar.rows.push_back(ChainedRowCon{.sibiling = this->bars[0].rows[i]});
        }
    }
}
