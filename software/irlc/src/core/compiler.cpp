#include "core/compiler.hpp"
#include "core/board_info.hpp"
#include "core/debug.hpp"
#include "core/netlist.hpp"
#include "core/parse.hpp"
#include "core/route.hpp"
#include "core/verify.hpp"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>

using namespace std;

int IrlCompiler::invoke() {

    int info_count = print_info();
    if (info_count != 0) {
        return 0;
    }

    if (!opts.input_file) {
        log_error("Input file not specified");
        return -1;
    }

    ifstream in_file(*opts.input_file);
    if (!in_file.is_open()) {
        log_error("Could not open input file: " + *opts.input_file);
        return -1;
    }

    std::string in_content(istreambuf_iterator<char>{in_file}, istreambuf_iterator<char>{});

    auto parsers = AllParsersFactory().make_parsers_prioritized(*opts.input_file, *this);

    unique_ptr<RawNetlist> netlist(nullptr);
    for (const INetlistParser *parser : parsers) {
        if (opts.should_verbose_parse()) {
            log_fd << "Attempting parser: " << parser->parser_name() << "\n";
        }
        try {
            netlist = std::move(parser->try_parse(*opts.input_file, in_content));
            break;
        } catch (runtime_error &e) {
            log_error(e.what());
        } catch (...) {
            log_error("Unknown error occured parsing.");
        }
    }

    if (netlist == nullptr) {
        log_error("Failed to parse input file: " + *opts.input_file);
        return -1;
    }

    if (!validate_simple_tspice(MAIN_BOARD)) {
        throw runtime_error("Bro we fucked up so bad oh jeez man this is bad :(");
    }
    SimpleTspiceRouter router(*this, MAIN_BOARD);

    router.prune_unconnected_nets(*netlist);

    if (opts.should_verbose_final_netlist()) {
        debug_print_netlist(log_fd, *netlist);
    }

    IrlVerifier verifier(*this);

    uint32_t violations = verifier.check_netlist_violations(*netlist);

    if (violations > 0) {
        log_error("Raw netlist fail validation rules");
        return -1;
    }

    unique_ptr<AssignedNetlist> assigned;
    if (opts.should_verbose_cell_assign()) {
        log_fd << "Attempting assignment";
    }

    try {
        assigned = router.try_assign(netlist);
    } catch (runtime_error &e) {
        log_error(e.what());
    } catch (...) {
        log_error("Unknown error occured parsing.");
    }

    if (assigned.get() == nullptr) {
        log_error("Failed to assign to standard cells");
    }

    try {
        router.make_connections(assigned);
    } catch (runtime_error &e) {
        log_error(e.what());
    } catch (...) {
        log_error("Unknown error occured parsing.");
    }

    return 0;
}

int IrlCompiler::print_info() {
    int printed = 0;

    // Printed by main
    if (opts.help)
        printed++;

    if (opts.version) {
        if (printed)
            cout << "\n===VERSION===\n";
        cout << "TODO, get a real version\n";
        printed++;
    }

    if (opts.show_components) {
        if (printed)
            cout << "\n===COMPONENTS===\n";
        printed++;

        int i = 0;
        cout << "{";
        for (auto &component : RECOGNIZED_COMPONENTS) {
            cout << "\n  \"" << component.long_name << "\": {\n    \"Referece Designators\": [";
            cout << '"' << component.spice_prefixes[0] << '"';
            for (int i = 1; i < component.spice_prefixes.size(); i++) {
                cout << ", \"" << component.spice_prefixes[i] << '"';
            }
            cout << "]";

            if (component.spice_values.size() > 0) {
                cout << ",\n    \"Values\": [";
                cout << '"' << component.spice_values[0] << '"';
                for (int i = 1; i < component.spice_values.size(); i++) {
                    cout << ", \"" << component.spice_values[i] << '"';
                }
                cout << "]";
            }
            i++;
            if (i == (sizeof(RECOGNIZED_COMPONENTS) / sizeof(RecognizedComponent)))
                cout << "\n  }\n";
            else
                cout << "\n  },\n";
        }
        cout << "}\n";
    }

    if (opts.show_nets) {
        if (printed)
            cout << "\n===NETS===\n";
        printed++;

        cout << "{\n";
        int i = 0;
        for (auto &net : RECOGNIZED_NETS) {
            cout << "  \"" << net_name(net.second) << "\": [ ";

            cout << '"' << net.first[0] << '"';
            for (int i = 1; i < net.first.size(); i++) {
                cout << ", \"" << net.first[i] << '"';
            }

            i++;
            if (i == (sizeof(RECOGNIZED_NETS) / sizeof(RECOGNIZED_NETS[0])))
                cout << " ]\n";
            else
                cout << " ],\n";
        }
        cout << "}\n";
    }

    return printed;
}
