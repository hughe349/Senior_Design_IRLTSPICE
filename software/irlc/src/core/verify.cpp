#include "core/verify.hpp"
#include "boost/graph/graph_concepts.hpp"
#include "util/macros.hpp"
#include <variant>

using namespace std;
using namespace boost;

uint32_t IrlVerifier::check_netlist_violations(RawNetlist const &netlist) {

    uint32_t violation_count = 0;

    pair verts = vertices(netlist);
    for (RawNetlist::vertex_descriptor v = *verts.first; verts.first != verts.second;
         verts.first++, v = *verts.first) {
        for (auto const &rule : RAW_VERTEX_RULES) {
            auto result = rule.rule(netlist, v);
            visit(overloads{[&](no_violation_t const &_) {
                                if (this->compiler.verbose()) {
                                    this->compiler.log_fd << "Rule " << rule.rule_name
                                                          << " PASSED by: " << netlist[v].name
                                                          << "\n";
                                }
                            },
                            [&](string msg) {
                                violation_count++;
                                this->compiler.log_fd << "VIOLATION at [" << netlist[v].name
                                                      << "] of rule [" << rule.rule_name << "]";
                                if (msg.size() > 0) {
                                    this->compiler.log_fd << ":\n    " << msg << "\n";
                                } else {
                                    this->compiler.log_fd << "\n";
                                }
                            }},
                  result);
        }
    }

    return violation_count;
}
