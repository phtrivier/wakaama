# cmake -DCMAKE_TOOLCHAIN_FILE=toolchain.cmake ../tests/secureclient

# this one is important
SET(CMAKE_SYSTEM_NAME Linux)

# specify the cross compiler
SET(CMAKE_C_COMPILER  /<openwrt_sdk>/staging_dir/toolchain-mips_gcc-4.6-linaro_uClibc-0.9.33.2/bin/mips-openwrt-linux-gcc)

# where is the target environment 
SET(CMAKE_FIND_ROOT_PATH  /<openwrt_sdk>/staging_dir/toolchain-mips_gcc-4.6-linaro_uClibc-0.9.33.2)

# search for programs in the build host directories
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
# for libraries and headers in the target directories
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
