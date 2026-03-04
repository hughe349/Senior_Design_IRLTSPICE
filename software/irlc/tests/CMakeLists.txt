include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.12.0
    GIT_SHALLOW 1
)
FetchContent_MakeAvailable(Catch2)

add_executable(irlc-tests
    ${CMAKE_CURRENT_LIST_DIR}/test_routing.cpp
    ${CMAKE_CURRENT_LIST_DIR}/test_boost.cpp
    ${CMAKE_CURRENT_LIST_DIR}/test_parsers.cpp
)

target_link_options(irlc-tests PRIVATE -g -fsanitize=address)

target_link_libraries(irlc-tests
    PRIVATE
        libirlc
        Boost::graph
        Catch2::Catch2WithMain
)

add_test(NAME irlc-tests COMMAND irlc-tests)
