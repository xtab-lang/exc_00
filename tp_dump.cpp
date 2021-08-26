#include "pch.h"
#include "tp_dump.h"

namespace exy {
void tp_dump::run(TpSymbol *moduleSymbol) {
    auto moduleNode = (TpModule*)moduleSymbol->node;
    traceln("dumping %tptype @thread(%u#0x#<green>)", &moduleNode->type, GetCurrentThreadId());
}
}