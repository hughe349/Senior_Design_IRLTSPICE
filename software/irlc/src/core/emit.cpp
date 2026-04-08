#include "core/emit.hpp"
#include "boost/asio/buffer.hpp"
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
#include <vector>

using namespace boost;
using namespace std;

static inline void send_bytes(ostream &os, uint8_t const *data, size_t n);

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

TspiceProgrammer::Result TspiceProgrammer::send_stream(ProgrammingInfo const &prog_info) {
    // Reset
    TspiceProgrammer::Result r = try_reset_board();
    if (!r)
        return r;

    vector<uint8_t> buffer{START_CONFIG, START_CONFIG};

    // Send 2 start bytes
    r = send(asio::buffer(buffer));
    if (!r)
        return r;

    buffer.clear();
    buffer.push_back(READY_TO_START);
    r = read_expecting(asio::buffer(buffer));
    buffer.clear();
    if (!r)
        return r;

    for (auto const &resistor : prog_info.resistances) {
        buffer.push_back(init_instr(START_POT, resistor.resistor_id));
        buffer.push_back(resistor.value);
        r = send(asio::buffer(buffer));
        if (!r)
            return r;
        buffer.clear();
        buffer.push_back(RETURN_TO_CONFIG);
        r = read_expecting(asio::buffer(buffer));
        buffer.clear();
        if (!r)
            return r;
        compiler.log_fd << "[OK]" << std::endl;
    }

    for (auto const &connection : prog_info.connections) {
        buffer.push_back(init_instr(START_CB, connection.crossbar_id));
        buffer.push_back(init_connetion(connection.col, connection.row));
        buffer.push_back(END_CB);
        r = send(asio::buffer(buffer));
        if (!r)
            return r;
        buffer.clear();
        buffer.push_back(RETURN_TO_CONFIG);
        r = read_expecting(asio::buffer(buffer));
        buffer.clear();
        if (!r)
            return r;
        compiler.log_fd << "[OK]" << std::endl;
    }

    compiler.log_fd << "END!" << std::endl;

    buffer.clear();
    buffer.push_back(END_CONFIG);
    r = send(asio::buffer(buffer));
    if (!r)
        return r;

    buffer.clear();
    buffer.push_back(CONFIG_SUCCESS);
    for (int i = 0; i < 1000; i++)
        buffer.push_back(0);

    r = read_expecting(asio::buffer(buffer));
    buffer.clear();
    if (!r)
        return r;

    return success_t{};
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

TspiceProgrammer::Result TspiceProgrammer::send(boost::asio::const_buffer const &buffer) {
    system::error_code error;

    // Sunc send
    if (compiler.opts.should_verbose_program()) {
        compiler.log_fd << "Sending: ";
        send_bytes(compiler.log_fd, (uint8_t const *)buffer.data(), buffer.size());
        compiler.log_fd << "...";
    }
    port.write_some(buffer, error);

    if (error.failed()) {
        if (error == asio::error::eof) {
            return ProgrammingError::port_closed();
        }
        return ProgrammingError(error);
    }

    return success_t{};
}

TspiceProgrammer::Result
TspiceProgrammer::read_expecting(boost::asio::mutable_buffer const &buffer) {

    compiler.log_fd << "Buffer size: " << buffer.size() << "\n";

    system::error_code error;
    bool read_complete = false;
    size_t read_n = 0;

    // Shoot me in the head VLAs don't exist in C++ so I gotta heap allocate for no reason.
    // Make it static for reuse
    static std::vector<uint8_t> temp_buff;
    temp_buff.assign((uint8_t *)buffer.data(), (uint8_t *)buffer.data() + buffer.size());

    asio::steady_timer timer(io, asio::chrono::milliseconds(timeout_ms));
    timer.async_wait([&](const system::error_code &ec) {
        if (!ec && !read_complete) {
            port.cancel();
        }
    });
    asio::async_read(port, buffer, [&](const system::error_code &ec, std::size_t n) {
        error = ec;
        read_n = n;
        if (ec.value() != system::errc::operation_canceled) {
            read_complete = true;
            timer.cancel();
        }
    });

    io.run();
    io.restart();

    compiler.log_fd << "Got (" << read_n << "): ";
    send_bytes(compiler.log_fd, (uint8_t *)buffer.data(), read_n);
    compiler.log_fd << std::endl;

    if (read_complete && !error) {
        if (read_n != buffer.size() ||
            std::memcmp(temp_buff.data(), buffer.data(), buffer.size())) {
            return ProgrammingError::bad_response(temp_buff.data(), buffer.size(),
                                                  (uint8_t *)buffer.data(), read_n);
        } else {
            return success_t{};
        }
    } else if (!read_complete) {
        return ProgrammingError::timeout_hit();
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
