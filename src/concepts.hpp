#pragma once

namespace mystd {

template <class T>
concept complete = requires { sizeof(T); };
}