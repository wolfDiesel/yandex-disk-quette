#include "auth/infrastructure/token_store.hpp"

namespace ydisquette {
namespace auth {

std::optional<std::string> TokenStore::getAccessToken() const {
    std::lock_guard lock(mutex_);
    if (accessToken_.empty()) return std::nullopt;
    return accessToken_;
}

void TokenStore::setTokens(std::string accessToken, std::string refreshToken) {
    std::lock_guard lock(mutex_);
    accessToken_ = std::move(accessToken);
    refreshToken_ = std::move(refreshToken);
}

void TokenStore::clear() {
    std::lock_guard lock(mutex_);
    accessToken_.clear();
    refreshToken_.clear();
}

std::optional<std::string> TokenStore::getRefreshToken() const {
    std::lock_guard lock(mutex_);
    if (refreshToken_.empty()) return std::nullopt;
    return refreshToken_;
}

}  // namespace auth
}  // namespace ydisquette
