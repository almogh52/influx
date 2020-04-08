#include <kernel/icxxabi.h>

atexit_func_entry_t atexit_funcs[ATEXIT_MAX_FUNCS];
uint8_t atexit_func_count = 0;

int __cxa_atexit(void (*f)(void *), void *objptr, void *dso) {
    // If the table is full
    if (atexit_func_count >= ATEXIT_MAX_FUNCS) {
        return -1;
    };

    // Save the function in the table
    atexit_funcs[atexit_func_count].destructor_func = f;
    atexit_funcs[atexit_func_count].obj_ptr = objptr;
    atexit_funcs[atexit_func_count].dso_handle = dso;
    atexit_func_count++;

    return 0;
};

void __cxa_finalize(void *f) {
    // The kernel asked to call all destructors
    if (!f) {
        for (uint8_t i = 0; i < atexit_func_count; i++) {
            // If the destructor isn't null, call it
            if (atexit_funcs[i].destructor_func != nullptr) {
                (*atexit_funcs[i].destructor_func)(atexit_funcs[i].obj_ptr);
            }
        }
    } else {
        for (uint8_t i = 0; i < atexit_func_count; i++) {
            // If the destructor is the wanted destructor, call it
            if (atexit_funcs[i].destructor_func == f) {
                (*atexit_funcs[i].destructor_func)(atexit_funcs[i].obj_ptr);
                atexit_funcs[i].destructor_func = 0;
            }
        }
    }
}