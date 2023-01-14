#ifndef DEBUG_H
#define DEBUG_H
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>
#include <cassert>


// TODO: 関数名表示したい
void print_trace() {
  void* callstack[128];
  int i, frames = backtrace(callstack, 128);
  char** strs = backtrace_symbols(callstack, frames);
  for (i = 0; i < frames; ++i) {
    printf("%s\n", strs[i]);
  }
  free(strs);
}

static void debug_assert(bool e) {
    if(!e) {
        print_trace();
    }
    assert(e);
}

#endif
