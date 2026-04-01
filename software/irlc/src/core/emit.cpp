#include "core/emit.hpp"
#include "boost/asio/io_context.hpp"
#include "boost/asio/serial_port.hpp"
#include <ranges>
#include <span>
#include <stdexcept>

using namespace boost;
using namespace std;

TspiceProgrammer::TspiceProgrammer(std::string_view serial_port, size_t baud, uint64_t timeout_ms,
                                   IrlCompiler const &compiler)
    : io(), port(asio::serial_port(io, serial_port.data())), timeout_ms(timeout_ms),
      compiler(compiler) {
    port.set_option(asio::serial_port::baud_rate(baud));
}

bool TspiceProgrammer::ping_board() {}

void TspiceProgrammer::reset_board() {}

void TspiceProgrammer::send_stream(ProgrammingInfo const &prog_info) {}
