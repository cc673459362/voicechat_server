cmake_minimum_required(VERSION 3.10)
project(voicechat_server LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug" CACHE STRING "Build type" FORCE)
endif()

option(ENABLE_ASAN "Enable AddressSanitizer/UndefinedBehaviorSanitizer" OFF)

if(ENABLE_ASAN)
  message(STATUS "Building with ASAN/UBSAN enabled")
  add_compile_options(-g -O1 -fno-omit-frame-pointer -fsanitize=address,undefined)
  add_link_options(-fsanitize=address,undefined)
else()
  add_compile_options(-g -O2 -Wall -Wextra -Wpedantic)
endif()

find_package(Threads REQUIRED)

# 自动收集 src 目录下的源码文件
file(GLOB_RECURSE SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx"
)

if(NOT SRC)
  message(FATAL_ERROR "No source files found in ${CMAKE_CURRENT_SOURCE_DIR}/src")
endif()

add_executable(chat_server ${SRC})

# include 目录指向 src（根据项目结构调整）
target_include_directories(chat_server PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_link_libraries(chat_server PRIVATE Threads::Threads)

install(TARGETS chat_server
        RUNTIME DESTINATION bin)

add_custom_target(run
    COMMAND ${CMAKE_COMMAND} --build ${CMAKE_BINARY_DIR} --target chat_server
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/logs
    COMMAND ${CMAKE_COMMAND} -E env LD_LIBRARY_PATH=${CMAKE_BINARY_DIR} ${CMAKE_BINARY_DIR}/chat_server
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    DEPENDS chat_server
    COMMENT "Build and run chat_server"
)

message(STATUS "Project: ${PROJECT_NAME}, Build type: ${CMAKE_BUILD_TYPE}")