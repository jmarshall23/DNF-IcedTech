project(UCC CXX)

add_executable(UCC UCC.cpp)
target_compile_definitions(UCC PRIVATE _UNICODE UNICODE)
target_link_libraries(UCC PRIVATE Engine)

target_include_directories(UCC PRIVATE
    ${CMAKE_SOURCE_DIR}/Core/Inc
    ${CMAKE_SOURCE_DIR}/Engine/Inc
)
