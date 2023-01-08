# Requirement
- C++/C compiler supporting C++20
- CMake
- libfmt (run `sudo apt install libfmt-dev` on Ubuntu)

# Build
```
cd linker
cmake -S . -B build
cmake --build build
```

# Run
```
./build/myld
```

# Test
```
./test.sh simple
```
