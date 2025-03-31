cmake_minimum_required(VERSION 3.10)
project(Timezones)

# Set C++ standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Define the source folder
set(SOURCE_DIR "src")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_BUILD_TYPE Release)
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Os -flto -ffunction-sections -fdata-sections")

# Collect all source files from the src directory
file(GLOB_RECURSE SOURCES "${SOURCE_DIR}/*.cpp")


# Create the executable
add_executable(${PROJECT_NAME} ${SOURCES})

# Include the source directory for headers
include_directories(${SOURCE_DIR})

