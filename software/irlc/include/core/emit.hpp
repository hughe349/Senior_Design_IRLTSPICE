#pragma once

#include "boost/asio/io_context.hpp"
#include "boost/asio/serial_port.hpp"
#include "core/compiler.hpp"
#include "core/route.hpp"
#include <cstdint>
#include <memory>
#include <ostream>
#include <string_view>
#include <tuple>

class TspiceProgrammer {

    boost::asio::io_context io;
    boost::asio::serial_port port;
    uint64_t timeout_ms;
    IrlCompiler const &compiler;

  public:
    // Constructors
    TspiceProgrammer(std::string_view serial_port, size_t baud, uint64_t timeout_ms,
                     IrlCompiler const &compiler);

    // Actual methods

    // Pings to see if board connected
    bool ping_board();
    // Resets the board
    void reset_board();
    // Sends stream of programming commands to the board, with timeouts
    void send_stream(ProgrammingInfo const &prog_info);
};
