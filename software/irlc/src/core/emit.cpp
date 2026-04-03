#include "core/emit.hpp"
#include "boost/asio/error.hpp"
#include "boost/asio/read.hpp"
#include "boost/system/detail/errc.hpp"
#include "extern/uart_bytes.h"

#include "boost/asio.hpp"

#include <cstdint>
#include <cstring>
#include <ostream>
#include <sstream>
#include <stdexcept>

using namespace boost;
using namespace std;

TspiceProgrammer::TspiceProgrammer(string_view serial_port, size_t baud, uint64_t timeout_ms,
                                   IrlCompiler const &compiler)
    : io(), port(asio::serial_port(io, serial_port.data())), timeout_ms(timeout_ms),
      compiler(compiler) {
    port.set_option(asio::serial_port::baud_rate(baud));
    port.set_option(asio::serial_port::parity(asio::serial_port::parity::none));
    port.set_option(asio::serial_port::flow_control(asio::serial_port::flow_control::none));
    port.set_option(asio::serial_port::stop_bits(asio::serial_port::stop_bits::one));
}

TspiceProgrammer::Result TspiceProgrammer::ping_board() {
    // Binary bloat w/ strings
    throw runtime_error("NOT IMPLEMENTED *COUGH* MICAH *COUGH*");
}

TspiceProgrammer::Result TspiceProgrammer::try_reset_board() {

    if (compiler.opts.should_verbose_program()) {
        compiler.log_fd << "Resetting...";
        flush(compiler.log_fd);
    }

    uint8_t minimal_buf = RESET_CONFIG;

    Result result = send(asio::buffer(&minimal_buf, 1));
    if (!result) {
        if (compiler.opts.should_verbose_program()) {
            compiler.log_fd << "[FAILED]\n";
        }
        return result;
    }
    if (compiler.opts.should_verbose_program()) {
        compiler.log_fd << "sent...";
        flush(compiler.log_fd);
    }

    minimal_buf = RESET_SUCCESS;
    result = read_expecting(asio::buffer(&minimal_buf, 1));
    if (!result) {
        if (compiler.opts.should_verbose_program())
            compiler.log_fd << "[FAILED]\n";
        return result;
    } else {
        if (compiler.opts.should_verbose_program())
            compiler.log_fd << "[OK]\n";
        return success_t{};
    }
}

TspiceProgrammer::Result TspiceProgrammer::send_stream(ProgrammingInfo const &prog_info) {}

TspiceProgrammer::Result TspiceProgrammer::send(boost::asio::const_buffer const &buffer) {
    system::error_code error;

    // Sunc send
    port.write_some(buffer, error);

    if (error.failed()) {
        if (error == asio::error::eof) {
            return ProgrammingError(PORT_CLOSED);
        }
        return ProgrammingError(error);
    }

    return success_t{};
}

TspiceProgrammer::Result
TspiceProgrammer::read_expecting(boost::asio::mutable_buffer const &buffer) {

    system::error_code error;
    bool read_complete = false;
    size_t read_n;

    uint8_t temp_buff[buffer.size()];

    std::memcpy(temp_buff, buffer.data(), buffer.size());

    asio::steady_timer timer(io, asio::chrono::milliseconds(timeout_ms));
    timer.async_wait([&](const system::error_code &ec) {
        if (!ec && !read_complete) {
            port.cancel();
        }
    });
    asio::async_read(port, buffer, [&](const system::error_code &ec, std::size_t n) {
        if (ec.value() != system::errc::operation_canceled) {
            error = ec;
            read_complete = true;
            read_n = n;
            timer.cancel();
        }
    });

    io.run();
    io.restart();

    if (read_complete && !error) {
        if (read_n != buffer.size() || std::memcmp(temp_buff, buffer.data(), buffer.size())) {
            return ProgrammingError::bad_response(temp_buff, buffer.size(),
                                                  (uint8_t *)buffer.data(), read_n);
        } else {
            return success_t{};
        }
    } else if (!read_complete) {
        return ProgrammingError(TIMEOUT_HIT);
    } else {
        return ProgrammingError(error);
    }
}

// Error stuff

static inline void send_bytes(ostream &os, uint8_t const *data, size_t n) {
    static char xtbl[] = "0123456789ABCDEF";
    os << "0x";
    if (n != 0) {
        os << xtbl[data[0] >> 4] << xtbl[data[0] & 0xF];
    }
    for (uint8_t const *dp = data + 1; dp != (data + n); dp++) {
        os << "," << xtbl[*dp >> 4] << xtbl[*dp & 0xF];
    }
}

ProgrammingError ProgrammingError::bad_response(uint8_t const *expected, size_t e_n,
                                                uint8_t const *got, size_t g_n) {
    ostringstream ss{};
    assert(e_n != 0);
    ss << "Bad response. Expected: ";
    send_bytes(ss, expected, e_n);
    ss << " and got: ";
    send_bytes(ss, got, g_n);
    return ProgrammingError(BAD_RESPONSE, ss.str());
}
