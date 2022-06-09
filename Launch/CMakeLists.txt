project(Launch C CXX)

add_executable(DukeForever WIN32
    Launch.cpp
    LaunchPrivate.h
    Res/LaunchRes.h
    Res/LaunchRes.rc
    Res/Unreal.ico
    Fortify.c
    Fortify.doc
    Fortify.h
    Ufortify.h
    Ufortify.hpp
    Zfortify.cpp
    Zfortify.hpp
    CrashHandler.h
    CriticalSection.h
    DiagAssert.h
    MemDumperValidator.h
    MemStress.h
    MSJDBG.h
    PSAPI.H
    SymbolEngine.h
    WarningsOff.h
    WarningsOn.h
    Res/Logo.bmp
)

target_include_directories(DukeForever PRIVATE ../Window/Inc ../Core/Inc ../Engine/Inc)
target_compile_definitions(DukeForever PRIVATE _UNICODE UNICODE)
target_link_libraries(DukeForever PRIVATE Engine dnAI dnGame D3DDrv Fire OpenALAudio)
target_link_directories(DukeForever PRIVATE ./)
