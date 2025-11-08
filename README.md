# winedll

This repository builds a standalone, universal version of Wine's msvcrt/ucrtbase using Clang.

The final DLL is a drop-in replacement for all Microsoft Visual C++ runtime libraries:
- msvcrt.dll
- msvcr70.dll
- msvcr80.dll
- msvcr90.dll
- msvcr100.dll
- msvcr110.dll
- msvcr120.dll
- msvcr140.dll
- ucrtbase.dll

## Building

```
cmake --preset debug
cmake --build --preset debug
```

The built DLL will be located at `build/debug/dlls/msvcrt/msvcrt.dll`.

Use `--preset release` to build a release binary to `build/release/dlls/msvcrt/msvcrt.dll`.

## License

The code in this repository is derived from Wine sources, which are licensed under LGPLv2.1+.

See [LICENSE](LICENSE) and [COPYING.LIB](COPYING.LIB) for details.
