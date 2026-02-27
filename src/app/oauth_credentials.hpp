#pragma once

#include "oauth_build_config.h"
#include <QByteArray>
#include <QString>

namespace ydisquette {

inline QString decodeObfuscated(const unsigned char* data, int len, unsigned char key) {
    if (!data || len <= 0) return QString();
    QByteArray b;
    b.reserve(len);
    for (int i = 0; i < len; ++i)
        b.append(static_cast<char>(data[i] ^ key));
    return QString::fromUtf8(b);
}

inline void getBuildTimeOAuthCredentials(QString* outClientId, QString* outClientSecret, QString* outRedirectUri) {
    if (outRedirectUri)
        *outRedirectUri = QStringLiteral(YDISQUETTE_DEFAULT_REDIRECT_URI);
    if (kObfuscatedIdLen > 0 && outClientId)
        *outClientId = decodeObfuscated(kObfuscatedId, kObfuscatedIdLen, kObfuscationKey);
    else if (outClientId)
        *outClientId = QString();
    if (kObfuscatedSecretLen > 0 && outClientSecret)
        *outClientSecret = decodeObfuscated(kObfuscatedSecret, kObfuscatedSecretLen, kObfuscationKey);
    else if (outClientSecret)
        *outClientSecret = QString();
}

}  // namespace ydisquette
