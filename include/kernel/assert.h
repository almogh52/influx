#pragma once

#include <kernel/panic.h>

#define kassert(expression)                                                              \
    ((expression) ? (void)0                                                              \
                  : kpanic("Assertion failed: %s, function: '%s', file: %s, line %d.\n", \
                           #expression, __PRETTY_FUNCTION__, __FILE__, __LINE__))
