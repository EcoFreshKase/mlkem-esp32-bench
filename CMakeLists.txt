cmake_minimum_required(VERSION 3.16)

# All source lives under src/.
# src/        → scanned for component dirs (finds src/main/)
# src/components/ → scanned for component dirs (finds mlkem/, benchmark/)
set(EXTRA_COMPONENT_DIRS
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/components"
)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(mlkem-esp32-bench)
