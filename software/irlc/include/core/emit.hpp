#pragma once

#include "boost/asio.hpp"
#include "boost/asio/buffer.hpp"
#include "boost/asio/registered_buffer.hpp"
#include "boost/outcome/result.hpp"
#include "core/compiler.hpp"
#include "core/route.hpp"
#include <cstdint>
#include <span>
#include <stdexcept>
#include <string_view>

typedef enum ProgrammingErrorKind {
    PORT_CLOSED,
    TIMEOUT_HIT,
    BAD_RESPONSE,
    OTHER_SYSTEM,
} ProgrammingErrorKind;

class ProgrammingError : public std::runtime_error {
    ProgrammingError(ProgrammingErrorKind kind, std::string str)
        : kind(kind), std::runtime_error(str) {};

  public:
    ProgrammingErrorKind kind;

    static ProgrammingError bad_response(uint8_t const *expected, size_t e_n, uint8_t const *got,
                                         size_t g_n);

    ProgrammingError(ProgrammingErrorKind kind)
        : kind(kind),
          std::runtime_error(std::string(kind == PORT_CLOSED   ? "Port closed by peer"
                                         : kind == TIMEOUT_HIT ? "Timeout hit"
                                                               : "Unknown error occured")) {}

    ProgrammingError(boost::system::error_code ec)
        : kind(OTHER_SYSTEM), std::runtime_error("System error: " + ec.message()) {}
};

typedef struct success_t {
} success_t;

class TspiceProgrammer {
  public:
    using Result = boost::outcome_v2::result<success_t, ProgrammingError>;

  private:
    boost::asio::io_context io;
    boost::asio::serial_port port;
    uint64_t timeout_ms;
    IrlCompiler const &compiler;

    bool send_w_timeout();

    Result send(boost::asio::const_buffer const &buffer);
    // Saves contents of buffer into a temp, then reads into buffer
    // Returns success iff buffer matches temp
    Result read_expecting(boost::asio::mutable_buffer const &buffer);

  public:
    // Constructors
    TspiceProgrammer(std::string_view serial_port, size_t baud, uint64_t timeout_ms,
                     IrlCompiler const &compiler);

    // Pings to see if board connected
    Result ping_board();
    // Resets the board.
    Result try_reset_board();
    // Sends stream of programming commands to the board, with timeouts
    Result send_stream(ProgrammingInfo const &prog_info);
};
