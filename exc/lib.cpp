#include "pch.h"

namespace lib {
void start() {
    exy::Heap::initialize();
    exy::aio::open();
}

void stop() {
    exy::aio::close();
    exy::Heap::dispose();
}
}