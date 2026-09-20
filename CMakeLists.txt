cmake_minimum_required(VERSION 3.21)
project(ControlPanel CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
        Slint
        GIT_REPOSITORY https://github.com/slint-ui/slint.git
        GIT_TAG release/1
        SOURCE_SUBDIR api/cpp
)
FetchContent_MakeAvailable(Slint)

add_executable(ControlPanel main.cpp
        sudoHelper.hpp
        utils/gpuUtil.hpp)

slint_target_sources(ControlPanel ui/main.slint)

target_link_libraries(ControlPanel PRIVATE Slint::Slint)