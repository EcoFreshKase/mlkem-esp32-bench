file(GLOB FIPS202_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

idf_component_register(
    SRCS ${FIPS202_SRCS}
    INCLUDE_DIRS "include"
)

# Switch to -O3 to measure the impact of maximum compiler optimisation on ML-KEM.
target_compile_options(${COMPONENT_LIB} PRIVATE -O2)
