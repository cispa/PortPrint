cmake_minimum_required(VERSION 3.14.0)
project (portprint C ASM)

set_property(GLOBAL PROPERTY C_STANDARD 23)
set_property(GLOBAL PROPERTY CXX_STANDARD 20)

include_directories(include)
add_compile_options("-O3;-msse3")
find_package(zstd REQUIRED)

add_executable(recorder
        "src/recorder.c"
        "src/compression.c"
        "src/contention_primitives/skylake.s"
        "src/contention_primitives/alder_lake_p.s"
        "src/contention_primitives/zen3.s"
        "src/victims/aesni-linux.S"
        "src/victims/aesni-openssl.s"
)
target_link_libraries(recorder PUBLIC zstd)
