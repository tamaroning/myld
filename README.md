# Myld
A toy linker which outputs an executable ELF for x86-64 processors

# Requirement
- C++/C compiler supporting C++20
- CMake
- libfmt (To install this, run `sudo apt install libfmt-dev`)

# Build
```
git clone <THIS REPO>
cd myld
cmake -S . -B build
cmake --build build
```
You can run `bear -- cmake -- build bulid` to crete compile_commands.json

# Run
```
./build/myld [OPTIONS] <OBJECT FILE1> [<OBJECT FILE2> ...]
```

# Test
```
./test.sh
```
This script runs all tests in tests/.

# Status

tests which should pass:
- simple1
- simple2
- simple3
- static1

tests which should fail
- static2
- static3

# Todo
- [x] executableの出力
- [ ] relocatableの出力
- [x] .strtabと.symtab生成
- [ ] linker script

### static link
- [x] 一つのオブジェクトファイルの入力
- [x] オブジェクトファイルのリロケーション (PLT32)
- [x] 複数オブジェクトファイルのリロケーション (PLT32)
- [ ] .bss

### dynamic link
- [ ] PLT, GOTを生成
