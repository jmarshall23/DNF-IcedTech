# WIP -- re-separate Core, Render, Window, WinDrv, IpDrv into their own projects
project(Core C CXX)

file(GLOB HEADERS ../Inc/*.h)
file(GLOB SOURCES *.cpp *.c)
add_library(Core STATIC ${HEADERS} ${SOURCES})
target_link_libraries(Core PRIVATE xcore)
target_compile_definitions(Core PRIVATE "_UNICODE" "UNICODE" "CORE_API=__declspec(dllexport)")
target_include_directories(Core PUBLIC ../Inc)
target_include_directories(Core PRIVATE
    ../../IpDrv/Src
    ../../WinDrv/Inc
    ../../Window/Inc
    ../../Engine/Inc
    ../Inc
    ${CMAKE_CURRENT_SOURCE_DIR}
)
