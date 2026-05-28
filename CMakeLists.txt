cmake_minimum_required(VERSION 3.18)
project(C-Mambo LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Source
file(GLOB_RECURSE SOURCE_FILES CONFIGURE_DEPENDS "Source/*")
add_executable(C-Mambo ${SOURCE_FILES})

# Call:
# mkdir Build
# cd Build
# cmake .. 
