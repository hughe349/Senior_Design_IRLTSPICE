#include "boost/program_options/value_semantic.hpp"
#include "core/routing.hpp"
#include <boost/program_options.hpp>
#include <cstddef>
#include <iostream>
#include <vector>

using std::string;
using std::vector;

namespace po = boost::program_options;

int main(int argc, char *argv[]) {

    po::options_description desc;
    desc.add_options()("help,h", "Show this help message.")("version,v", "Print version and exit.")(
        "input-file", po::value<string>(), "Input spice file to compile");
    po::positional_options_description p;
    p.add("input-file", -1);

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help") > 0) {
        std::cout << "Usage:\n\n"
                  << "  irlc [OPTIONS] <input-file>\n\n";
        std::cout << "Options:\n\n" << desc << std::endl;
        return -1;
    }

    return 0;
}
