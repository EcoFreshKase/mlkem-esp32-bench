idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES benchmark fips202
)

# Adjust optimisation level independently of the mlkem and benchmark components.
target_compile_options(${COMPONENT_LIB} PRIVATE -O2)
