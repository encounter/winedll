`libclang_rt.builtins-i386.a` was built from LLVM sources (commit b4b57adb8) using:

```
cmake -S compiler-rt -G Ninja -B build/compiler-rt \
    -DCMAKE_C_COMPILER=clang \
    -DCMAKE_CXX_COMPILER=clang++ \
    -DCMAKE_ASM_COMPILER_TARGET=i686-pc-windows-gnu \
    -DCMAKE_C_COMPILER_TARGET=i686-windows-gnu \
    -DCMAKE_CXX_COMPILER_TARGET=i686-pc-windows-gnu \
    -DCMAKE_SYSTEM_NAME=Windows \
    -DCOMPILER_RT_BUILD_BUILTINS=ON \
    -DCOMPILER_RT_BUILD_LIBFUZZER=OFF \
    -DCOMPILER_RT_BUILD_MEMPROF=OFF \
    -DCOMPILER_RT_BUILD_PROFILE=OFF \
    -DCOMPILER_RT_BUILD_SANITIZERS=OFF \
    -DCOMPILER_RT_BUILD_XRAY=OFF \
    -DCOMPILER_RT_DEFAULT_TARGET_ONLY=ON \
    -DCOMPILER_RT_INCLUDE_TESTS=OFF
cmake --build build/compiler-rt --target builtins --verbose
```
