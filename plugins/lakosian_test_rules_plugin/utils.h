#ifndef LKS_TEST_RULES_PLG_UTILS_H
#define LKS_TEST_RULES_PLG_UTILS_H

#include <string>
#include <vector>

namespace utils {

struct string {
    explicit string(std::string const& s): _s(s)
    {
    }

    [[nodiscard]] constexpr bool contains(std::string const& s) const noexcept
    {
        return _s.find(s) != std::string::npos;
    }

    [[nodiscard]] constexpr bool startswith(std::string const& s) const noexcept
    {
        return _s.find(s) == 0;
    }

    [[nodiscard]] constexpr bool endswith(std::string const& s) const noexcept
    {
        return _s.find(s) == _s.size() - s.size();
    }

    [[nodiscard]] std::vector<std::string> split(char c) const noexcept
    {
        auto result = std::vector<std::string>{};
        auto substr = std::string{""};
        for (auto&& _c : _s) {
            if (_c == c) {
                result.emplace_back(substr);
                substr = "";
                continue;
            }
            substr += _c;
        }
        result.emplace_back(substr);
        return result;
    }

  private:
    std::string const& _s;
};

} // namespace utils

#endif
