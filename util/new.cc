#include "util/new.h"

void* operator new(size_t, void* ptr) { return ptr; }

