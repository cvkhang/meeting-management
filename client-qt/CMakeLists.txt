cmake_minimum_required(VERSION 3.10)
project(MeetingClient VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 REQUIRED COMPONENTS Widgets Network)

set(HEADERS
    include/networkmanager.h
    include/mainwindow.h
    include/loginwidget.h
    include/studentwidget.h
    include/teacherwidget.h
    include/serverconfigdialog.h
    include/groupdetaildialog.h
)

set(SOURCES
    src/main.cpp
    src/networkmanager.cpp
    src/mainwindow.cpp
    src/loginwidget.cpp
    src/studentwidget.cpp
    src/teacherwidget.cpp
    src/serverconfigdialog.cpp
    src/groupdetaildialog.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(${PROJECT_NAME} PRIVATE Qt5::Widgets Qt5::Network pthread)
