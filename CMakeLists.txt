cmake_minimum_required(VERSION 3.16)
project(statik VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# macOS homebrew LLVM path
set(CMAKE_PREFIX_PATH "/opt/homebrew/opt/llvm")

find_package(LLVM REQUIRED CONFIG)
find_package(Clang REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")

add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})
include_directories(${CLANG_INCLUDE_DIRS})
include_directories(include)

llvm_map_components_to_libnames(llvm_libs support core)

add_executable(statik
    src/main.cpp
    src/ASTVisitor.cpp
    src/LoopVisitor.cpp
    src/DependencyAnalyzer.cpp
    src/ArrayDependencyAnalyzer.cpp
    src/PointerAnalyzer.cpp
    src/FunctionCallAnalyzer.cpp
    src/CrossIterationAnalyzer.cpp
)

target_link_libraries(statik
    clangAST
    clangBasic
    clangDriver
    clangFrontend
    clangTooling
    ${llvm_libs}
)

# Disable unused parameter warnings during development
target_compile_options(statik PRIVATE -Wall -Wno-unused-parameter)