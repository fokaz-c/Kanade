#pragma once
#include <string>

namespace Core::Util {

/**
 * @brief Generates a cryptographically random, URL-safe string.
 * This is the 'code_verifier'.
 */
std::string generate_code_verifier(size_t length = 64);

/**
 * @brief Hashes and Base64-URL-encodes the verifier to create the
 * 'code_challenge'. 'code_challenge'.
 * @param verifier The string from generate_code_verifier().
 */
std::string generate_code_challenge(const std::string& verifier);

} // namespace Core::Util
