cmake_minimum_required(VERSION 3.12)
project(VectorDB)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(FAISS_DIR "${CMAKE_SOURCE_DIR}/external/faiss-install" CACHE PATH "FAISS installation directory")

include_directories(${FAISS_DIR}/include)

link_directories(${FAISS_DIR}/lib)

find_package(OpenMP)

include(FetchContent)

FetchContent_Declare(json
    URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz
)
FetchContent_MakeAvailable(json)

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

add_executable(data_preprocess data_preprocess.cpp)
target_link_libraries(data_preprocess 
    nlohmann_json::nlohmann_json
    llama
)

add_executable(vector_db vector_db.cpp)
target_link_libraries(vector_db 
    nlohmann_json::nlohmann_json
    faiss
    openblas
)

if(OpenMP_CXX_FOUND)
    target_link_libraries(data_preprocess OpenMP::OpenMP_CXX)
    target_link_libraries(vector_db OpenMP::OpenMP_CXX)
endif()

message(STATUS "FAISS: ${FAISS_DIR}")
message(STATUS "OpenMP: ${OpenMP_CXX_FOUND}")