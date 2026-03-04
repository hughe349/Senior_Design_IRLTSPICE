#pragma once

#include <array>
#include <optional>
#include <ostream>
#include <string>

enum IrlCompilerOptionGroup {
    GENERAL,
    PARSING,
    INFO,
};
static std::array IRL_COMPILER_OPTION_GROUPS = {GENERAL, PARSING, INFO};
static inline const char *ilrCompilerOptionGroupName(IrlCompilerOptionGroup o) {
    switch (o) {
    case GENERAL:
        return "General";
        break;
    case PARSING:
        return "Netlist Parsing";
        break;
    case INFO:
        return "Info Displaying";
        break;
    }
}

#define INFILE_OPT input_file

typedef enum unspecified_t { UNSPECIFIED } unspecified_t;

#undef FLAG_OPT
#undef TYPED_OPT
#define COMPILER_OPTIONS                                                                           \
    FLAG_OPT(help, "h", "Show this help message and exit", GENERAL)                                \
    FLAG_OPT(version, "", "Print version and exit", GENERAL)                                       \
    TYPED_OPT(INFILE_OPT, "", std::optional<std::string>, std::nullopt,                            \
              "Input spice file to compile", GENERAL)                                              \
    FLAG_OPT(show_components, "", "List the references + values of recognized components", INFO)   \
    FLAG_OPT(verbose, "v", "Enable verbose output", GENERAL)

struct IrlCompilerOptions {
#define FLAG_OPT(LONG_N, SHORT_N, MSG, CATEGORY) bool LONG_N;
#define TYPED_OPT(LONG_N, SHORT_N, TYPE, DEFAULT, MSG, CATEGORY) TYPE LONG_N;

    COMPILER_OPTIONS

#undef FLAG_OPT
#undef TYPED_OPT
};

class IrlCompiler {
  public:
    IrlCompilerOptions opts;
    std::ostream &log_fd;

    IrlCompiler(IrlCompilerOptions &&opts, std::ostream &log_fd) : opts(opts), log_fd(log_fd) {}

    // Run the compiler.  Does everything.
    // All other functions are a subset of this function.
    // Returns a return code
    int invoke();

    // Prints all messages demanded by options
    // Returns the number of messages printed
    int print_info();

    // Helpers
    inline bool verbose() const { return this->opts.verbose; };

    inline void log_error(std::string const &msg) { this->log_fd << "[ERROR] - " << msg << "\n"; }
};

#define QUOTE(X) #X
#define EX_AND_QUOTE(X) QUOTE(X)
