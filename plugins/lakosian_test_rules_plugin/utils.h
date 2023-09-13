#ifndef LKS_TEST_RULES_PLG_UTILS_H
#define LKS_TEST_RULES_PLG_UTILS_H

#include <string>

namespace utils {
struct string {
    explicit string(std::string const& s): _s(s)
    {
    }

    [[nodiscard]] constexpr bool endswith(std::string const& s) const noexcept
    {
        return _s.find(s) == _s.size() - s.size();
    }

  private:
    std::string const& _s;
};
} // namespace utils

#endif
