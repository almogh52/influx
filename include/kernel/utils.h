#pragma once

namespace influx {
namespace utils {
template <typename C, void (C::*M)()>
void method_function_wrapper(void* p) {
    (static_cast<C*>(p)->*M)();
}
}  // namespace utils
}  // namespace influx