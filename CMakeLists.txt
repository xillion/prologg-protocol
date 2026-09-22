# libs/prologg/CMakeLists.txt
cmake_minimum_required(VERSION 3.10)
project(prologg
        VERSION 0.0.1
        LANGUAGES C CXX ASM)

# 1. Setup Paths
set(DISPATCH_GEN_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/generate_dispatch.py")
set(GEN_H "${CMAKE_CURRENT_BINARY_DIR}/prologg_dispatch.h")
set(GEN_CPP "${CMAKE_CURRENT_BINARY_DIR}/prologg_dispatch.cpp")

# 2. Find Nanopb Generator (Assuming nanopb is in your path or project)
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/nanopb/extra)
find_package(Nanopb REQUIRED)
nanopb_generate_cpp(TARGET proto prologg.proto)

add_custom_command(
    OUTPUT "${GEN_H}" "${GEN_CPP}"
    # Use the discovered interpreter variable
    COMMAND Python3::Interpreter "${DISPATCH_GEN_SCRIPT}" "${CMAKE_CURRENT_BINARY_DIR}/prologg.pb.h" "${CMAKE_CURRENT_SOURCE_DIR}/templates" "${CMAKE_CURRENT_BINARY_DIR}"
    # Depend on the generator script and the specific header it needs to scan
    DEPENDS "${DISPATCH_GEN_SCRIPT}" "${CMAKE_CURRENT_BINARY_DIR}/prologg.pb.h" "${CMAKE_CURRENT_SOURCE_DIR}/templates/prologg_dispatch.h.j2" "${CMAKE_CURRENT_SOURCE_DIR}/templates/prologg_dispatch.cpp.j2"
    COMMENT "Generating protocol dispatch table from tags"
    VERBATIM
)

# 4. Library Definition
add_library(prologg STATIC
    ${GEN_CPP}
)

# 5. Include Directories
target_include_directories(prologg PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/nanopb
)

# # 6. Link Libraries
target_link_libraries(prologg PUBLIC proto mbed-core-flags) # This is needed so