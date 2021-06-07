#include "pch.h"

namespace exy {
bool Configuration::initialize() {
    traceln("Searching for configuration file...");
    return compiler->errors == 0;
}

void Configuration::dispose() {

}
} // namespace exy