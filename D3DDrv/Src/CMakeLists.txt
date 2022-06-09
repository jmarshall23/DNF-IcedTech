project(D3DDrv CXX)

file(GLOB D3DDrvFiles
     "*.h"
     "*.cpp"    
)

add_library(D3DDrv SHARED ${D3DDrvFiles})
target_compile_definitions(D3DDrv PRIVATE _UNICODE UNICODE)

target_link_directories(D3DDrv PRIVATE ../dxsdk/lib/x86)
target_link_libraries(D3DDrv PRIVATE Engine d3d9.lib legacy_stdio_definitions)

target_include_directories(D3DDrv PRIVATE
    ../../Core/Inc
    ../../Engine/Inc
    ../../Render/Inc
    ../dxsdk/include
)
