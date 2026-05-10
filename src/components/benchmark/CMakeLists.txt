file(GLOB BENCH_SRCS "benchmark.c" "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

idf_component_register(
    SRCS ${BENCH_SRCS}
    INCLUDE_DIRS "include"
    REQUIRES mlkem
    PRIV_REQUIRES esp_hw_support   # provides esp_cpu.h / esp_cpu_get_cycle_count()
)

# Benchmark glue code does not benefit from aggressive optimisation; keep at -O2
# so call overhead stays predictable and doesn't obscure measured cycle counts.
target_compile_options(${COMPONENT_LIB} PRIVATE -O2)
