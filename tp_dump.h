#pragma once

#include "tp.h"

namespace exy {
struct Typer;
struct tp_dump {
    void dispose() {}

    void run(TpSymbol *moduleSymbol);
};
}