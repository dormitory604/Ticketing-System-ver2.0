cmake_minimum_required(VERSION 3.16)
project(ticket-system VERSION 0.1 LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Network Sql)

# 启用 Qt 的自动元对象编译器、UI 编译器和资源编译器
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# 设置 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 添加子项目
add_subdirectory(client-app)
add_subdirectory(server-app)
add_subdirectory(admin-app)
