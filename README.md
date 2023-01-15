# Myld
a linker which output an executable for AMD x86-64 processor

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

- [x] simple1
    - convert an elf with no relocation into an executable
- [ ] simple2
    - convert an elf with relocations into an executable
    - TODO
        - .rela.text/.rel.text(Elf64_Rela/Elf64_Rel)を読んでrelocationを保持
        - セグメント内のシンボルなら、相対アドレスなので簡単に置き換えられる。
- [ ] static1
    convert multiple elf with relocations into an executable
    - TODO

# memo

単一オブジェクトふぁいるからexeへの変換
```
tamaron@PC:~/work/linker$ readelf tests/simple2/simple2.o -s -S -r
There are 11 section headers, starting at offset 0x1b8:

Section Headers:
  [Nr] Name              Type             Address           Offset
       Size              EntSize          Flags  Link  Info  Align
  [ 0]                   NULL             0000000000000000  00000000
       0000000000000000  0000000000000000           0     0     0
  [ 1] .text             PROGBITS         0000000000000000  00000040
       000000000000003a  0000000000000000  AX       0     0     1
  [ 2] .rela.text        RELA             0000000000000000  00000140
       0000000000000018  0000000000000018   I       8     1     8
  [ 3] .data             PROGBITS         0000000000000000  0000007a
       0000000000000000  0000000000000000  WA       0     0     1
  [ 4] .bss              NOBITS           0000000000000000  0000007a
       0000000000000000  0000000000000000  WA       0     0     1
  [ 5] .comment          PROGBITS         0000000000000000  0000007a
       000000000000002c  0000000000000001  MS       0     0     1
  [ 6] .note.GNU-stack   PROGBITS         0000000000000000  000000a6
       0000000000000000  0000000000000000           0     0     1
  [ 7] .note.gnu.pr[...] NOTE             0000000000000000  000000a8
       0000000000000020  0000000000000000   A       0     0     8
  [ 8] .symtab           SYMTAB           0000000000000000  000000c8
       0000000000000060  0000000000000018           9     2     8
  [ 9] .strtab           STRTAB           0000000000000000  00000128
       0000000000000014  0000000000000000           0     0     1
  [10] .shstrtab         STRTAB           0000000000000000  00000158
       000000000000005d  0000000000000000           0     0     1
Key to Flags:
  W (write), A (alloc), X (execute), M (merge), S (strings), I (info),
  L (link order), O (extra OS processing required), G (group), T (TLS),
  C (compressed), x (unknown), o (OS specific), E (exclude),
  D (mbind), l (large), p (processor specific)

Relocation section '.rela.text' at offset 0x140 contains 1 entry:
  Offset          Info           Type           Sym. Value    Sym. Name + Addend
000000000026  000200000004 R_X86_64_PLT32    0000000000000000 f - 4

Symbol table '.symtab' contains 4 entries:
   Num:    Value          Size Type    Bind   Vis      Ndx Name
     0: 0000000000000000     0 NOTYPE  LOCAL  DEFAULT  UND 
     1: 0000000000000000     0 FILE    LOCAL  DEFAULT  ABS simple2.c
     2: 0000000000000000    24 FUNC    GLOBAL DEFAULT    1 f
     3: 0000000000000018    34 FUNC    GLOBAL DEFAULT    1 _start
```
to
```
There are 7 section headers, starting at offset 0x1148:

Section Headers:
  [Nr] Name              Type             Address           Offset
       Size              EntSize          Flags  Link  Info  Align
  [ 0]                   NULL             0000000000000000  00000000
       0000000000000000  0000000000000000           0     0     0
  [ 1] .text             PROGBITS         0000000000080000  00001000
       000000000000003a  0000000000000000  AX       0     0     1
  [ 2] .note.gnu.pr[...] NOTE             0000000000080040  00001040
       0000000000000020  0000000000000000   A       0     0     8
  [ 3] .comment          PROGBITS         0000000000000000  00001060
       000000000000002b  0000000000000001  MS       0     0     1
  [ 4] .symtab           SYMTAB           0000000000000000  00001090
       0000000000000060  0000000000000018           5     2     8
  [ 5] .strtab           STRTAB           0000000000000000  000010f0
       0000000000000014  0000000000000000           0     0     1
  [ 6] .shstrtab         STRTAB           0000000000000000  00001104
       000000000000003d  0000000000000000           0     0     1
Key to Flags:
  W (write), A (alloc), X (execute), M (merge), S (strings), I (info),
  L (link order), O (extra OS processing required), G (group), T (TLS),
  C (compressed), x (unknown), o (OS specific), E (exclude),
  D (mbind), l (large), p (processor specific)

There are no relocations in this file.

Symbol table '.symtab' contains 4 entries:
   Num:    Value          Size Type    Bind   Vis      Ndx Name
     0: 0000000000000000     0 NOTYPE  LOCAL  DEFAULT  UND 
     1: 0000000000000000     0 FILE    LOCAL  DEFAULT  ABS simple2.c
     2: 0000000000080000    24 FUNC    GLOBAL DEFAULT    1 f
     3: 0000000000080018    34 FUNC    GLOBAL DEFAULT    1 _start
```
