#pragma once
#include <string>

namespace Core::Util {

/**
 * @brief Generates a cryptographically random, URL-safe string.
 * This is the 'code_verifier'.
 */
std::string generate_code_verifier(size_t length = 64);

/**
<<<<<<< HEAD
 * @brief Hashes and Base64-URL-encodes the verifier to create the 'code_challenge'.
=======
 * @brief Hashes and Base64-URL-encodes the verifier to create the
 * 'code_challenge'.
>>>>>>> 967ffe1 (core/utils: (feat)Add PKCE implementation for Spotify OAuth)
 * @param verifier The string from generate_code_verifier().
 */
std::string generate_code_challenge(const std::string& verifier);

<<<<<<< HEAD
} // namespace core::util
=======
} // namespace Core::Util
>>>>>>> 967ffe1 (core/utils: (feat)Add PKCE implementation for Spotify OAuth)
