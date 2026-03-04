#include "core/compiler.hpp"
#include "core/debug.hpp"
#include "core/netlist.hpp"
#include "core/parse.hpp"
#include "core/route.hpp"
#include "core/verify.hpp"
#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>

using namespace std;

int IrlCompiler::invoke() {

    int info_count = this->print_info();
    if (info_count != 0) {
        return 0;
    }

    if (!this->opts.input_file) {
        log_error("Input file not specified");
        return -1;
    }

    ifstream in_file(*(this->opts.input_file));
    if (!in_file.is_open()) {
        log_error("Could not open input file: " + *(this->opts.input_file));
        return -1;
    }

    std::string in_content(istreambuf_iterator<char>{in_file}, istreambuf_iterator<char>{});

    auto parsers = AllParsersFactory().make_parsers_prioritized(*(this->opts.input_file));

    unique_ptr<RawNetlist> netlist(nullptr);
    for (const INetlistParser *parser : parsers) {
        if (this->verbose()) {
            cout << "Attempting parser: " << parser->parser_name() << "\n";
        }
        try {
            netlist = std::move(parser->try_parse(*(this->opts.input_file), in_content));
            break;
        } catch (ParseException e) {
            log_error(e.what());
        } catch (...) {
            log_error("Unknown error occured parsing.");
        }
    }

    if (netlist == nullptr) {
        log_error("Failed to parse input file: " + *(this->opts.input_file));
        return -1;
    }

    prune_unconnected_nets(*netlist);

    debug_print_netlist(cout, *netlist);

    IrlVerifier verifier(*this);

    uint32_t violations = verifier.check_netlist_violations(*netlist);

    if (violations > 0) {
        log_error("Raw netlist failes validation rules");
    }

    return 0;
}

int IrlCompiler::print_info() {
    int printed = 0;

    // Printed by main
    if (this->opts.help)
        printed++;

    if (this->opts.version) {
        if (printed)
            cout << "\n===VERSION===\n";
        cout << "TODO, get a real version\n";
        printed++;
    }

    if (this->opts.show_components) {
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

    return printed;
}
