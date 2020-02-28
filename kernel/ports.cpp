#include <kernel/ports.h>

template <typename T>
void influx::ports::out(T value, uint16_t port) {
    __asm__ __volatile__("out %[port], %[value]" : : [ port ] "dN"(port), [ value ] "a"(value));
}

template <typename T>
T influx::ports::in(uint16_t port) {
    T value;

    __asm__ __volatile__("in %[value], %[port]" : [ value ] "=a"(value) : [ port ] "dN"(port));

    return value;
}

template void influx::ports::out(uint8_t value, uint16_t port);
template void influx::ports::out(uint16_t value, uint16_t port);
template void influx::ports::out(uint32_t value, uint16_t port);

template uint8_t influx::ports::in(uint16_t port);
template uint16_t influx::ports::in(uint16_t port);
template uint32_t influx::ports::in(uint16_t port);