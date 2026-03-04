#include "core/compiler.hpp"
#include "core/netlist.hpp"
#include <iostream>
#include <string>

using namespace std;

int IrlCompiler::invoke() {

    int info_count = this->print_info();
    if (info_count != 0) {
        return 0;
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
