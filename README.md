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
You can run `bear -- cmake -- build bulid` to crete compile_commands.json

# Run
```
./build/myld <OBJECT FILE>
```

# Test
```
./test.sh simple
```
