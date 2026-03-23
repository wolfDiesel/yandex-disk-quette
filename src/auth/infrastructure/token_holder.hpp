#pragma once

#include "auth/application/itoken_provider.hpp"
#include <optional>
#include <string>

namespace ydisquette {
namespace auth {

struct TokenHolder : ITokenProvider {
    std::optional<std::string> token;
    std::optional<std::string> getAccessToken() const override { return token; }
};

}  // namespace auth
}  // namespace ydisquette
