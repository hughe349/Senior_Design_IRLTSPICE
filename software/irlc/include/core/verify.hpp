#pragma once

#include "boost/graph/filtered_graph.hpp"
#include "boost/graph/graph_concepts.hpp"
#include "core/compiler.hpp"
#include "core/netlist.hpp"
#include "util/boost_util.hpp"
#include <cstdint>
#include <ranges>
#include <string>
#include <variant>

typedef enum no_violation_t { NO_VIOLATION } no_violation_t;
// typedef enum no_violation_t {
//     NO_VIOLATION
// } no_violation_t;

typedef std::variant<no_violation_t, std::string> RuleViolationResult;

typedef RuleViolationResult (*VertexRuleViolates_t)(RawNetlist const &netlist, RawVert v);

typedef RuleViolationResult (*GraphRuleViolates_t)(RawNetlist const &netlist);

struct RawNetlistVertexRule {
    const char *rule_name;
    VertexRuleViolates_t rule;
};

struct RawNetlistGraphRule {
    const char *rule_name;
    GraphRuleViolates_t rule;
};

class IrlVerifier {
    IrlCompiler const &compiler;

  public:
    IrlVerifier(IrlCompiler const &compiler) : compiler(compiler) {};

    // Returns the number of rule violations logged
    uint32_t check_netlist_violations(RawNetlist const &netlist);
};

constexpr std::array RAW_GRAPH_RULES = {
    RawNetlistGraphRule{.rule_name = "Circuit contains output",
                        .rule = [](RawNetlist const &netlist) -> RuleViolationResult {
                            for (auto v : pair_to_iter(boost::vertices(netlist))) {
                                if (netlist[v].kind == NET &&
                                    netlist[v].value.net_value == RawNetlistVertexInfo::OUTPUT) {
                                    return NO_VIOLATION;
                                }
                            }
                            return "";
                        }},
    RawNetlistGraphRule{.rule_name = "Circuit contains input",
                        .rule = [](RawNetlist const &netlist) -> RuleViolationResult {
                            for (auto v : pair_to_iter(boost::vertices(netlist))) {
                                if (netlist[v].kind == NET &&
                                    netlist[v].value.net_value == RawNetlistVertexInfo::INPUT) {
                                    return NO_VIOLATION;
                                }
                            }
                            return "";
                        }},
};

constexpr std::array RAW_VERTEX_RULES = {
    RawNetlistVertexRule{
        .rule_name = "Opamp supply plus must be V_HIGH or unconnectd",
        .rule = [](RawNetlist const &netlist, RawVert v) -> RuleViolationResult {
            if (netlist[v].kind != OPAMP)
                return NO_VIOLATION;

            for (auto e : pair_to_iter(boost::out_edges(v, netlist))) {

                RawNetlistVertexInfo const &target_info = netlist[boost::target(e, netlist)];
                assert(target_info.kind == NET);

                if (netlist[e].kind == PIN_OPAMP_SUPPLY_PLUS) {
                    if (target_info.value.net_value != RawNetlistVertexInfo::V_HIGH) {
                        return "";
                    }
                }
            }

            return NO_VIOLATION;
        },
    },
    RawNetlistVertexRule{
        .rule_name = "Opamp supply minus must be V_NEG or unconnectd",
        .rule = [](RawNetlist const &netlist, RawVert v) -> RuleViolationResult {
            if (netlist[v].kind != OPAMP)
                return NO_VIOLATION;

            for (auto e : pair_to_iter(boost::out_edges(v, netlist))) {

                RawNetlistVertexInfo const &target_info = netlist[boost::target(e, netlist)];
                assert(target_info.kind == NET);

                if (netlist[e].kind == PIN_OPAMP_SUPPLY_MINUS) {
                    if (target_info.value.net_value != RawNetlistVertexInfo::V_NEG) {
                        return "";
                    }
                }
            }

            return NO_VIOLATION;
        },
    },
    RawNetlistVertexRule{
        .rule_name = "Passive components should not be bypassed",
        .rule = [](RawNetlist const &netlist, RawVert v) -> RuleViolationResult {
            if (netlist[v].kind == R || netlist[v].kind == C || netlist[v].kind == DIODE) {
            } else {
                return NO_VIOLATION;
            }

            if (boost::out_degree(v, netlist) != 2) {
                return "Passive component must be connected to 2 nets";
            }

            std::pair edges = boost::out_edges(v, netlist);
            auto e1 = *edges.first;
            auto e2 = *(edges.first + 1);

            auto t1 = boost::target(e1, netlist);
            auto t2 = boost::target(e2, netlist);

            if (t1 == t2) {
                return "Component bypassed by net: " + netlist[t1].name;
            }

            return NO_VIOLATION;
        },
    },
    RawNetlistVertexRule{
        .rule_name = "Circuit output must be driven by only a buffer",
        .rule = [](RawNetlist const &netlist, RawVert v) -> RuleViolationResult {
            if (netlist[v].kind != NET ||
                netlist[v].value.net_value != RawNetlistVertexInfo::OUTPUT) {
                return NO_VIOLATION;
            }

            if (boost::in_degree(v, netlist) > 1) {
                std::string out = "Output driven by multiple nets: [";
                auto iter = pair_to_iter(boost::in_edges(v, netlist));
                out += "\"";
                out += netlist[boost::source(*iter.begin(), netlist)].name;
                out += "\"";
                iter.advance(1);
                for (auto e : iter) {
                    out += "\", ";
                    out += netlist[boost::source(e, netlist)].name;
                    out += "\"";
                }
                out += "]";
                return out;
            }

            if (netlist[v].kind == NET &&
                netlist[v].value.net_value == RawNetlistVertexInfo::OUTPUT) {
                auto e = *boost::in_edges(v, netlist).first;
                auto const &edge = netlist[e];
                auto const &src = netlist[boost::source(e, netlist)];
                if (src.kind != CELL_BUFFER) {
                    return "Output driven by non-cell-buffer: " + src.name;
                } else if (edge.kind != PIN_CELL_BUFFER_OUT) {
                    return "Output driven by input of cell buffer instead of output: " + src.name;
                }
            }

            return NO_VIOLATION;
        },
    }};
