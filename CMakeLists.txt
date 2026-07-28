cmake_minimum_required(VERSION 3.20)

project(ProfitabilityTool LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Ubuntu's llvm-18 package references zstd::libzstd_shared in its exported
# CMake target list, but doesn't ship a zstd CMake package config to
# provide it. Manually stub the imported target so LLVMConfig.cmake's
# link-interface check succeeds -- the actual .so is present on the
# system even though CMake can't discover it via find_package(zstd).
find_library(ZSTD_LIBRARY NAMES zstd libzstd)
if (ZSTD_LIBRARY AND NOT TARGET zstd::libzstd_shared)
    add_library(zstd::libzstd_shared UNKNOWN IMPORTED)
    set_target_properties(zstd::libzstd_shared PROPERTIES
        IMPORTED_LOCATION "${ZSTD_LIBRARY}")
endif()

# LLVM
find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

# Clang
find_package(Clang REQUIRED CONFIG)

message(STATUS "Using ClangConfig.cmake in: ${Clang_DIR}")

include_directories(
    ${LLVM_INCLUDE_DIRS}
    ${CLANG_INCLUDE_DIRS}
)

add_definitions(${LLVM_DEFINITIONS})

add_executable(ProfitabilityTool
    src/main.cpp
    src/ASTVisitor.cpp
    src/ASTConsumer.cpp
    src/FrontendAction.cpp
    src/LoopInfo.cpp
    src/LoopManager.cpp
    src/ASTFeatureExtractor.cpp
    src/ProfitabilityRegion.cpp
    src/RegionDetector.cpp
    src/PragmaCallbacks.cpp
    src/RegionOutliner.cpp
    src/LoopUtils.cpp
    src/IRFeaturePass.cpp
    src/IRExtractionDriver.cpp
    src/CSVWriter.cpp
)

llvm_map_components_to_libnames(llvm_libs
    core
    irreader
    analysis
    support
    asmparser
)

target_link_libraries(ProfitabilityTool
    PRIVATE
    clangTooling
    clangToolingCore
    clangFrontend
    clangSerialization
    clangDriver
    clangParse
    clangSema
    clangAnalysis
    clangAST
    clangBasic
    clangLex
)
