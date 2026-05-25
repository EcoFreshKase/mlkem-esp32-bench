idf_component_register(
    SRCS "tests.c" "fips202_tests.c" "mlkem_tests.c"
    INCLUDE_DIRS "include"
    REQUIRES fips202 mlkem
)

target_compile_options(${COMPONENT_LIB} PRIVATE -O2)
