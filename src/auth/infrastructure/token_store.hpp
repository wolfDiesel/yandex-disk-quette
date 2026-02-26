#pragma once

#include "auth/application/itoken_provider.hpp"
#include <mutex>
#include <optional>
#include <string>

namespace ydisquette {
namespace auth {

class TokenStore : public ITokenProvider {
public:
    std::optional<std::string> getAccessToken() const override;
    void setTokens(std::string accessToken, std::string refreshToken);
    void clear();
    std::optional<std::string> getRefreshToken() const;

private:
    mutable std::mutex mutex_;
    std::string accessToken_;
    std::string refreshToken_;
};

}  // namespace auth
}  // namespace ydisquette
