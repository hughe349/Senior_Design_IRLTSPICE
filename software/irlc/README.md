# IRLTSPICE Compiler - IRLC

## Setup
1. To build/setup env run:
    ```bash
    mkdir build
    cmake . -B build
    cmake --build build
    ```
1. To test run:
    ```bash
    ctest --test-dir build
    ```

## Dependencies

### Libraries:
- [Boost - 1.90](www.boost.org/)
- [Catch2 - 3.12.0](https://github.com/catchorg/Catch2)

### Tooling:
- cmake >= 4.0
