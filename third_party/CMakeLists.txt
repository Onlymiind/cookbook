cmake_minimum_required(VERSION 3.29)

project(cookbook C CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(SQLITE_SRC 
	third_party/sqlite/sqlite3.c
	third_party/sqlite/sqlite3.h
)

add_library(sqlite3 ${SQLITE_SRC})

set(COOKBOOK_SRC
	main.cpp
)

add_subdirectory(Catch2)

add_executable(cookbook ${COOKBOOK_SRC})
target_include_directories(cookbook PRIVATE third_party)
target_link_libraries(cookbook PRIVATE sqlite3)
