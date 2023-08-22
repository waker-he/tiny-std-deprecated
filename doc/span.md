# span
- [code](../src/span.hpp)
- [my note about `std::span`](https://github.com/waker-umich/cs-learning-notes/blob/main/cpp/c%2B%2B20/ranges/ranges.md#spans)
- `span` is trivially copyable
- just used raw pointer as contiguous iterator, need to make generalization after iterator classes are implemented
- need to implement a constructor that takes a contiguous range as argument
