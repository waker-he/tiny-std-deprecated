#pragma once

namespace mystl {

template <class T>
concept complete = requires { sizeof(T); };
}