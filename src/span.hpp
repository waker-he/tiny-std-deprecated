#pragma once

#include <limits>
#include <type_traits>

namespace mystd {

inline constexpr std::size_t dynamic_extent =
    std::numeric_limits<std::size_t>::max();

template <class T, std::size_t Extent = dynamic_extent> class span {
  public:
    static constexpr std::size_t extent = Extent;

    // member types
    using element_type = T;
    using size_type = std::size_t;
    using reference = T &;
    using pointer = T *;

    // span is trivially copyable
    constexpr span(const span &) noexcept = default;
    constexpr auto operator=(const span &) noexcept -> span & = default;
    constexpr ~span() = default;

    // regular constructors
    constexpr span() noexcept
        requires(extent == 0) || (extent == dynamic_extent)
        : _begin{nullptr} {
        if constexpr (extent == dynamic_extent) {
            _sz = 0;
        }
    }

    constexpr span(pointer first, size_type count) noexcept : _begin(first) {
        if constexpr (extent == dynamic_extent) {
            _sz = count;
        }
    }

    constexpr span(pointer first, pointer last) noexcept : _begin(first) {
        if constexpr (extent == dynamic_extent) {
            _sz = last - first;
        }
    }

    // element access
    constexpr auto data() const noexcept -> pointer { return _begin; }
    constexpr auto operator[](size_type idx) const noexcept -> reference {
        return *(_begin + idx);
    }

    // observers
    constexpr auto size() const noexcept -> size_type {
        if constexpr (extent == dynamic_extent) {
            return _sz;
        } else {
            return extent;
        }
    }

    constexpr auto empty() const noexcept -> bool { return size() == 0; }

    // subviews
    template <size_type Offset, size_type Count = dynamic_extent>
        requires(Offset <= Extent) &&
                (Count == dynamic_extent || Extent - Offset >= Count)
                constexpr auto subspan() const noexcept
                -> span<element_type,
                        Count == dynamic_extent ? Extent - Offset : Count> {
        return span < element_type,
               Count == dynamic_extent
                   ? Extent - Offset
                   : Count > (_begin + Offset, Count == dynamic_extent
                                                   ? Extent - Offset
                                                   : Count);
    }

    constexpr auto subspan(size_type offset,
                           size_type count = dynamic_extent) const noexcept
        -> span<element_type> {
        return span<element_type>(
            _begin + offset, count == dynamic_extent ? extent - offset : count);
    }

    template <size_type Count>
    constexpr auto first() const noexcept -> span<element_type, Count> {
        return subspan<0, Count>();
    }

    constexpr auto first(size_type count) const noexcept -> span<element_type> {
        return subspan(0, count);
    }

    template <size_type Count>
    constexpr auto last() const noexcept -> span<element_type, Count> {
        if constexpr (extent == dynamic_extent) {
            return span<element_type, Count>(_begin + _sz - Count, Count);
        } else {
            return subspan<Extent - Count, Count>();
        }
    }

    constexpr auto last(size_type count) const noexcept -> span<element_type> {
        if constexpr (extent == dynamic_extent) {
            return subspan(_sz - count, count);
        } else {
            return subspan(extent - count, count);
        }
    }

  private:
    pointer _begin;
    struct _empty {};
    [[no_unique_address]] std::conditional_t<extent == dynamic_extent,
                                             size_type, _empty>
        _sz;
};

} // namespace mystd