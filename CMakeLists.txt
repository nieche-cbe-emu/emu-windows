cmake_minimum_required(VERSION 3.21)
project(NiecheEmu LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)

# 模拟核心是 emu-core-rs 的 C ABI（nieche.dll + nieche.h）。
# NIECHE_DIR 指向那个仓库；默认按同级目录找。
set(NIECHE_DIR "${CMAKE_SOURCE_DIR}/../rust" CACHE PATH "emu-core-rs 的位置")

qt_add_executable(NiecheEmu WIN32
    src/main.cpp
    src/mainwindow.cpp
    src/mainwindow.h
    src/core.cpp
    src/core.h
    src/screenview.cpp
    src/screenview.h
    src/keypad.cpp
    src/keypad.h
)
target_include_directories(NiecheEmu PRIVATE "${NIECHE_DIR}/emuffi")
target_link_libraries(NiecheEmu PRIVATE Qt6::Widgets)

if (MSVC)
    # **主线程栈必须加大。** unicorn 的代码生成很吃栈，MSVC 默认只给 1MB，
    # 而模拟器就跑在界面线程上——溢出的表现是进程直接消失，
    # 没有异常、没有对话框、没有日志。Python 那版也栽在同一处。
    target_link_options(NiecheEmu PRIVATE "/STACK:8388608")
endif()
