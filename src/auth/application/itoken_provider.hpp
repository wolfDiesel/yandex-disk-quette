#pragma once

#include <optional>
#include <string>

namespace ydisquette {
namespace auth {

struct ITokenProvider {
    virtual ~ITokenProvider() = default;
    virtual std::optional<std::string> getAccessToken() const = 0;
};

}  // namespace auth
}  // namespace ydisquette
