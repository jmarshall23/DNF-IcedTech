project(Fire CXX)

add_library(Fire SHARED
    FractalPrivate.h
    UnFire.asm
    UnFireP2.asm
    UnFractal.cpp
    UnWater.asm
)

target_compile_definitions(Fire PRIVATE _UNICODE UNICODE)
target_link_libraries(Fire PRIVATE Engine)

target_include_directories(Fire PRIVATE
    ../../Core/Inc
    ../../Engine/Inc
)
