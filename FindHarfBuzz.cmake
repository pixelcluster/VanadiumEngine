# Since HarfBuzz is compiled directly into the VanadiumEngine lib, there is no need to link another library,
# as all HarfBuzz functions are already contained in the VanadiumEngine lib.
set(HarfBuzz_LIBRARY "")

set(HarfBuzz_INCLUDE_DIRS "${CMAKE_SOURCE_DIR}/dependencies/harfbuzz/src")
# Hardcode for now
set(HarfBuzz_VERSION "4.2.0")
set(HarfBuzz_FOUND TRUE)