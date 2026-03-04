#pragma once

#include "core/compiler.hpp"
#include "core/netlist.hpp"
#include <cstdint>
#include <string>
#include <variant>

typedef enum no_violation_t { NO_VIOLATION } no_violation_t;
// typedef enum no_violation_t {
//     NO_VIOLATION
// } no_violation_t;

typedef std::variant<no_violation_t, std::string> RuleViolationResult;

typedef RuleViolationResult (*VertexRuleViolates_t)(RawNetlist const &netlist,
                                                    RawNetlist::vertex_descriptor v);

struct RawNetlistVertexRule {
    const char *rule_name;
    VertexRuleViolates_t rule;
};

class IrlVerifier {
    IrlCompiler const &compiler;

  public:
    IrlVerifier(IrlCompiler const &compiler) : compiler(compiler) {};

    // Returns the number of rule violations logged
    uint32_t check_netlist_violations(RawNetlist const &netlist);
};

constexpr std::array RAW_VERTEX_RULES = {
    RawNetlistVertexRule{
        .rule_name = "Opamp supply plus must be V_HIGH or unconnectd",
        .rule = [](RawNetlist const &netlist,
                   RawNetlist::vertex_descriptor v) -> RuleViolationResult {
            if (netlist[v].kind != OPAMP)
                return NO_VIOLATION;

            // std::string err_msg = "";

            std::pair edges = boost::out_edges(v, netlist);
            for (auto e = edges.first; e != edges.second; e++) {

                RawNetlistVertexInfo const &target_info = netlist[boost::target(*e, netlist)];
                assert(target_info.kind == NET);

                if (netlist[*e].kind == PIN_OPAMP_SUPPLY_PLUS) {
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
        .rule = [](RawNetlist const &netlist,
                   RawNetlist::vertex_descriptor v) -> RuleViolationResult {
            if (netlist[v].kind != OPAMP)
                return NO_VIOLATION;

            std::pair edges = boost::out_edges(v, netlist);
            for (auto e = edges.first; e != edges.second; e++) {

                RawNetlistVertexInfo const &target_info = netlist[boost::target(*e, netlist)];
                assert(target_info.kind == NET);

                if (netlist[*e].kind == PIN_OPAMP_SUPPLY_MINUS) {
                    if (target_info.value.net_value != RawNetlistVertexInfo::V_NEG) {
                        return "";
                    }
                }
            }

            return NO_VIOLATION;
        },
    }

};
