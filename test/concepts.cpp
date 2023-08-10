
#include "concepts.hpp"

using namespace mystl;

struct incomplete_t;

struct complete_t {};

static_assert(not complete<incomplete_t>);
static_assert(complete<complete_t>);

auto main() -> int {}