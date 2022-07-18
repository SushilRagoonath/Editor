cmake_minimum_required(VERSION 3.14)
project(editor)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# C++ version
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_subdirectory(blah)
add_executable(editor
		source/main.cpp
)

include_directories(
	blah/include
	imgui/
	source/
)
target_link_libraries(editor blah)
