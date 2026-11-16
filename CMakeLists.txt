cmake_minimum_required(VERSION 3.16)
project(obs-scripture-dock VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_AUTOMOC ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBOBS REQUIRED libobs)
pkg_check_modules(SWORD sword)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

add_library(obs-scripture-dock MODULE
    src/plugin-main.cpp
    src/sword_downloader.hpp
)

set_target_properties(obs-scripture-dock PROPERTIES PREFIX "")

target_link_libraries(obs-scripture-dock PRIVATE
    ${LIBOBS_LIBRARIES}
    obs-frontend-api
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

target_include_directories(obs-scripture-dock PRIVATE
    ${LIBOBS_INCLUDE_DIRS}
    /usr/include/obs
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(SWORD_FOUND)
    target_compile_definitions(obs-scripture-dock PRIVATE HAVE_SWORD=1)
    target_link_libraries(obs-scripture-dock PRIVATE ${SWORD_LIBRARIES})
    target_include_directories(obs-scripture-dock PRIVATE ${SWORD_INCLUDE_DIRS})
endif()
