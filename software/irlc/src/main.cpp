#include "boost/program_options/options_description.hpp"
#include "boost/program_options/parsers.hpp"
#include "boost/program_options/value_semantic.hpp"
#include "core/compiler.hpp"
#include <boost/program_options.hpp>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <map>

using std::string;

namespace po = boost::program_options;

consteval std::string flagify(const char *long_name) {
    std::string str(long_name);
    std::replace(str.begin(), str.end(), '_', '-');
    return str;
}

IrlCompilerOptions build_compiler_options(std::map<std::string, po::variable_value> &vm);

int main(int argc, char *argv[]) {

    std::map<IrlCompilerOptionGroup, po::options_description> opt_groups{};
    for (auto opt : IRL_COMPILER_OPTION_GROUPS) {
        opt_groups.emplace(opt, po::options_description(ilrCompilerOptionGroupName(opt)));
    }

    // Actually the coolest pattern I've ever seen
#define FLAG_OPT(LONG_N, SHORT_N, MSG, CATEGORY)                                                   \
    do {                                                                                           \
        opt_groups[CATEGORY].add_options()((flagify(QUOTE(LONG_N)) + "," SHORT_N).c_str(), MSG);   \
    } while (0);
#define TYPED_OPT(LONG_N, SHORT_N, TYPE, DEFAULT, MSG, CATEGORY)                                   \
    do {                                                                                           \
        opt_groups[CATEGORY].add_options()((flagify(QUOTE(LONG_N)) + "," SHORT_N).c_str(),         \
                                           po::value<TYPE>(), MSG);                                \
                                                                                                   \
    } while (0);

    // Like wtf even is this
    COMPILER_OPTIONS

#undef FLAG_OPT
#undef TYPED_OPT

    po::options_description cmdline_desc;

    for (auto &pair : opt_groups) {
        cmdline_desc.add(pair.second);
    }

    po::positional_options_description positional;
    positional.add(flagify(EX_AND_QUOTE(INFILE_OPT)).c_str(), -1);

    po::variables_map vm;
    po::store(
        po::command_line_parser(argc, argv).options(cmdline_desc).positional(positional).run(), vm);
    po::store(po::parse_command_line(argc, argv, cmdline_desc), vm);
    po::notify(vm);

    IrlCompilerOptions opts = build_compiler_options(vm);

    IrlCompiler compiler(std::move(opts), std::cout);

    if (opts.help) {
        vm["help"].as<string>();
        std::cout << "Usage:\n\n"
                  << "  irlc [OPTIONS] <input-file>\n\n";
        std::cout << "Options:" << cmdline_desc << std::endl;
        return -1;
    }

    return compiler.invoke();
}

IrlCompilerOptions build_compiler_options(std::map<std::string, po::variable_value> &vm) {

    // #define FLAG_OPT(LONG_N, SHORT_N, MSG, CATEGORY) \
    //     std::cout << QUOTE(LONG_N) << " - count: " << vm.count(flagify(QUOTE(LONG_N)).c_str()) <<
    //     "\n";
    // #define TYPED_OPT(LONG_N, SHORT_N, TYPE, MSG, CATEGORY) \
    //     std::cout << QUOTE(LONG_N) << " - count: " << vm.count(flagify(QUOTE(LONG_N)).c_str()) <<
    //     "\n"; COMPILER_OPTIONS
    // #undef FLAG_OPT
    // #undef TYPED_OPT

#define FLAG_OPT(LONG_N, SHORT_N, MSG, CATEGORY)                                                   \
    .LONG_N = vm.contains(flagify(QUOTE(LONG_N)).c_str()),
#define TYPED_OPT(LONG_N, SHORT_N, TYPE, DEFAULT, MSG, CATEGORY)                                   \
    .LONG_N = vm.contains(flagify(QUOTE(LONG_N)).c_str())                                          \
                  ? vm[flagify(QUOTE(LONG_N)).c_str()].as<TYPE>()                                  \
                  : DEFAULT,

    return {COMPILER_OPTIONS};

#undef FLAG_OPT
#undef TYPED_OPT
}
