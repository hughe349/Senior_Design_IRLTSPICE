include(FetchContent)

FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG v3.12.0
    GIT_SHALLOW 1
)
FetchContent_MakeAvailable(Catch2)

add_executable(irlc-tests
    ./test_boost.cpp
)

target_link_libraries(irlc-tests
    PRIVATE
        Boost::graph
        Catch2::Catch2WithMain
)

add_test(NAME irlc-tests COMMAND irlc-tests)
