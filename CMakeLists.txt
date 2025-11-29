cmake_minimum_required(VERSION 3.12)
project(VectorDatabase)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Paths
set(FAISS_DIR "${CMAKE_SOURCE_DIR}/external/faiss-install" CACHE PATH "FAISS installation directory")

# Include directories
include_directories(${FAISS_DIR}/include)

# Link directories
link_directories(${FAISS_DIR}/lib)

# Find OpenMP
find_package(OpenMP)

# Use FetchContent to download dependencies
include(FetchContent)

# Fetch nlohmann/json
FetchContent_Declare(json
    URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
)
FetchContent_MakeAvailable(json)

# Fetch llama.cpp
FetchContent_Declare(
    llama
    GIT_REPOSITORY https://github.com/ggerganov/llama.cpp.git
    GIT_TAG master
    GIT_SHALLOW TRUE
)

set(LLAMA_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(LLAMA_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)

FetchContent_MakeAvailable(llama)

# Part 1: Data preprocessing
add_executable(data_preprocess data_preprocess.cpp)
target_link_libraries(data_preprocess 
    nlohmann_json::nlohmann_json
    llama
)

# Part 1: Vector database
add_executable(vector_db vector_db.cpp)
target_link_libraries(vector_db 
    nlohmann_json::nlohmann_json
    faiss
    openblas
)

# Part 2: Main RAG system
add_executable(main main.cpp)
target_link_libraries(main
    nlohmann_json::nlohmann_json
    faiss
    openblas
    llama
)

# Add OpenMP if available
if(OpenMP_CXX_FOUND)
    target_link_libraries(data_preprocess OpenMP::OpenMP_CXX)
    target_link_libraries(vector_db OpenMP::OpenMP_CXX)
    target_link_libraries(main OpenMP::OpenMP_CXX)
endif()

# Show configuration
message(STATUS "=== Build Configuration ===")
message(STATUS "FAISS: ${FAISS_DIR}")
message(STATUS "OpenMP: ${OpenMP_CXX_FOUND}")
message(STATUS "==========================")

# Installation
install(TARGETS data_preprocess vector_db main
    RUNTIME DESTINATION bin
)