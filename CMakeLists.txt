cmake_minimum_required(VERSION 3.16)
project(obs-scripture-dock VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

if(UNIX AND NOT APPLE)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBOBS REQUIRED libobs)
    pkg_check_modules(SWORD sword)
    set(OBS_INC ${LIBOBS_INCLUDE_DIRS} /usr/include/obs)
    set(OBS_LIBS ${LIBOBS_LIBRARIES} obs-frontend-api)
else()
    # Windows / macOS: pass -DOBS_INCLUDE_DIRS=... -DOBS_LIBRARIES="obs;obs-frontend-api" -DOBS_LIB_DIRS=...
    set(OBS_INC ${OBS_INCLUDE_DIRS})
    set(OBS_LIBS ${OBS_LIBRARIES})
    if(OBS_LIB_DIRS)
        link_directories(${OBS_LIB_DIRS})
    endif()
endif()

add_library(obs-scripture-dock MODULE
    src/plugin-main.cpp
    src/sword_downloader.hpp
)

set_target_properties(obs-scripture-dock PROPERTIES PREFIX "")

target_link_libraries(obs-scripture-dock PRIVATE
    ${OBS_LIBS}
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

target_include_directories(obs-scripture-dock PRIVATE
    ${OBS_INC}
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(SWORD_FOUND)
    target_compile_definitions(obs-scripture-dock PRIVATE HAVE_SWORD=1)
    target_link_libraries(obs-scripture-dock PRIVATE ${SWORD_LIBRARIES})
    target_include_directories(obs-scripture-dock PRIVATE ${SWORD_INCLUDE_DIRS})
endif()
