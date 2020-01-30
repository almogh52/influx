#pragma once

#include <kernel/panic.h>

#define kassert(check) do { if(!(check)) kpanic(); } while(0)
