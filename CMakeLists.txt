cmake_minimum_required(VERSION 3.20)
project(ingest C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_compile_options(-Wall -Wextra -O2)


include(FetchContent)
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz)
FetchContent_MakeAvailable(json)
FetchContent_Declare(SQLiteCpp GIT_REPOSITORY https://github.com/SRombauts/SQLiteCpp.git)
FetchContent_MakeAvailable(SQLiteCpp)
FetchContent_Declare(sqlite3 URL https://sqlite.org/2026/sqlite-amalgamation-3530400.zip) 
FetchContent_MakeAvailable(sqlite3)
#add_library(sqlite3 STATIC ${sqlite3_SOURCE_DIR}/sqlite3.c)
#find_package(Threads REQUIRED)
#target_include_directories(sqlite3 PUBLIC ${sqlite3_SOURCE_DIR})
#target_link_libraries(sqlite3 PRIVATE Threads::Threads ${CMAKE_DL_LIBS})



set(SQLITECPP_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(SQLITECPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(SQLITECPP_RUN_CPPCHECK OFF CACHE BOOL "" FORCE)
set(SQLITECPP_RUN_CPPLINT OFF CACHE BOOL "" FORCE)
set(SQLITECPP_INTERNAL_SQLITE ON CACHE BOOL "" FORCE)


add_executable(ingest src/ingest.cpp)
target_link_options(ingest PRIVATE -static-libgcc -static-libstdc++)
target_link_libraries(ingest PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(ingest PRIVATE SQLiteCpp sqlite3)

add_executable(report src/report.cpp)
target_link_options(report PRIVATE -static-libgcc -static-libstdc++)
target_link_libraries(report PRIVATE nlohmann_json::nlohmann_json)
target_link_libraries(report PRIVATE SQLiteCpp sqlite3)
