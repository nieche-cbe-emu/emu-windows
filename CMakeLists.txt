cmake_minimum_required(VERSION 3.21)
project(NiecheEmu LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

# 模拟核心是 emu-core-rs 的 C ABI（nieche.dll + nieche.h）。
# NIECHE_DIR 指向那个仓库；默认按同级目录找。
set(NIECHE_DIR "${CMAKE_SOURCE_DIR}/../rust" CACHE PATH "emu-core-rs 的位置")

# app.rc 给 exe 本身的图标；app.qrc 把同一个文件打进资源，供 QIcon 用。
qt_add_executable(NiecheEmu WIN32
    app.rc
    app.qrc
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/core.cpp
    src/core.h
    src/emuthread.cpp
    src/emuthread.h
    src/screenview.cpp
    src/screenview.h
    src/keypad.cpp
    src/keypad.h
    src/audioout.cpp
    src/audioout.h
)
target_include_directories(NiecheEmu PRIVATE "${NIECHE_DIR}/emuffi")
target_link_libraries(NiecheEmu PRIVATE Qt6::Widgets)
if (WIN32)
    # MCI（出声）在 winmm 里。系统自带的 GS 合成器是唯一能放 .mid 的东西。
    target_link_libraries(NiecheEmu PRIVATE winmm)
endif()

if (MSVC)
    # **主线程栈必须加大。** unicorn 的代码生成很吃栈，MSVC 默认只给 1MB，
    # 而模拟器就跑在界面线程上——溢出的表现是进程直接消失，
    # 没有异常、没有对话框、没有日志。Python 那版也栽在同一处。
    target_link_options(NiecheEmu PRIVATE "/STACK:8388608")
endif()

# 排查用的最小探针：不带 Qt，只 LoadLibrary + nieche_selftest。
# 用来把"闪退是 Qt 引起的"和"是链接设置/进程本身引起的"分开。
add_executable(probe src/probe.c)
if (MSVC)
    target_link_options(probe PRIVATE "/STACK:8388608")
endif()

# 按键回归测试（emu-windows#1）。默认不建，CI 用 -DNIECHE_TESTS=ON 打开。
# 不需要 nieche.dll：核心加载失败时外壳照常起界面，按键链路与核心无关。
option(NIECHE_TESTS "构建 Qt 外壳的回归测试" OFF)
if (NIECHE_TESTS)
    enable_testing()
    find_package(Qt6 REQUIRED COMPONENTS Test)
    # 用 add_executable 而不是 qt_add_executable：测试要的是控制台程序，
    # ctest 才读得到输出
    add_executable(keys_test
        tests/keys_test.cpp
        src/mainwindow.cpp
        src/mainwindow.h
        src/core.cpp
        src/core.h
        src/emuthread.cpp
        src/emuthread.h
        src/screenview.cpp
        src/screenview.h
        src/keypad.cpp
        src/keypad.h
        src/audioout.cpp
        src/audioout.h
    )
    set_target_properties(keys_test PROPERTIES WIN32_EXECUTABLE FALSE)
    target_include_directories(keys_test PRIVATE src "${NIECHE_DIR}/emuffi")
    target_link_libraries(keys_test PRIVATE Qt6::Widgets Qt6::Test)
    if (WIN32)
        target_link_libraries(keys_test PRIVATE winmm)
    endif()
    if (MSVC)
        target_link_options(keys_test PRIVATE "/STACK:8388608")
    endif()
    # -o -,txt：QtTest 的逐条结果直接打到标准输出，失败时 ctest 日志里看得到
    add_test(NAME keys COMMAND keys_test -o -,txt)
endif()
