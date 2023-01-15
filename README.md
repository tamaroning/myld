# Myld
A toy linker which outputs an executable ELF for x86-64 processors

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

# Todo
- [x] executableの出力
- [ ] relocatableの出力

### static link
- [x] 一つのオブジェクトファイルの入力
- [ ] 複数のオブジェクトファイルの入力
- [x] 単一オブジェクトファイルのリロケーション (PLT32)
- [ ] 複数オブジェクトファイルのリロケーション

### dynamic link
- [ ] PLT, GOTを生成
