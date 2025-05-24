cmake_minimum_required(VERSION 3.25)
project(clicker LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -O3")

option(VALIDATION "enable validation layer" OFF)
option(VK_NO_PROTOTYPES "dynamically link with vulkan" ON)

if(VALIDATION)
    add_compile_definitions(VALIDATION)
endif()

if(VK_NO_PROTOTYPES)
    add_compile_definitions(VK_NO_PROTOTYPES)
endif()

if(WIN32)
    add_compile_definitions(_CRT_SECURE_NO_DEPRECATE)
endif()

set(SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/src/main.c
    ${CMAKE_CURRENT_LIST_DIR}/src/mathext.c
    ${CMAKE_CURRENT_LIST_DIR}/src/dynArray.c
    ${CMAKE_CURRENT_LIST_DIR}/src/sdl.c
    ${CMAKE_CURRENT_LIST_DIR}/src/vkInit.c
    ${CMAKE_CURRENT_LIST_DIR}/src/pipeline.c
    ${CMAKE_CURRENT_LIST_DIR}/src/garbage.c
    ${CMAKE_CURRENT_LIST_DIR}/src/vkFunctions.c
    ${CMAKE_CURRENT_LIST_DIR}/src/util.c
    ${CMAKE_CURRENT_LIST_DIR}/src/game.c

    ${CMAKE_CURRENT_LIST_DIR}/external/src/stb_image.c
)

find_package(SDL3 REQUIRED)
find_package(Vulkan REQUIRED)
find_package(freetype CONFIG REQUIRED)

add_executable(${CMAKE_PROJECT_NAME} ${SOURCES})
target_link_libraries(${CMAKE_PROJECT_NAME} SDL3::SDL3 Vulkan::Headers freetype)
if(UNIX)
    target_link_libraries(${CMAKE_PROJECT_NAME} m)
endif()
if(NOT VK_NO_PROTOTYPES)
    target_link_libraries(${CMAKE_PROJECT_NAME} Vulkan::Vulkan)
endif()
target_include_directories(${CMAKE_PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include ${CMAKE_CURRENT_LIST_DIR}/external/include )

target_compile_definitions(${CMAKE_PROJECT_NAME} PRIVATE CMAKE_EXPORT_COMPILE_COMMANDS=ON)

if(WIN32)
    add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:${CMAKE_PROJECT_NAME}> COMMAND_EXPAND_LISTS $<TARGET_FILE_DIR:${CMAKE_PROJECT_NAME}>)
endif()

add_custom_command(TARGET ${CMAKE_PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_LIST_DIR}/assets ${CMAKE_CURRENT_BINARY_DIR}/assets)