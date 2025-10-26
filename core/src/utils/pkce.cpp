#include "utils/pkce.h"
<<<<<<< HEAD
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <openssl/bio.h>
#include <openssl/evp.h>
#include <openssl/buffer.h>
#include <stdexcept>
#include <vector>
#include <algorithm>
#include <iomanip>
#include <sstream>
=======
#include <algorithm>
#include <iomanip>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <sstream>
#include <stdexcept>
#include <vector>
>>>>>>> 967ffe1 (core/utils: (feat)Add PKCE implementation for Spotify OAuth)

namespace Core::Util {

std::string base64_url_encode(const std::vector<unsigned char>& data) {
<<<<<<< HEAD
    BIO *bio, *b64;
    BUF_MEM *bufferPtr;

    b64 = BIO_new(BIO_f_base64());
    bio = BIO_new(BIO_s_mem());
    bio = BIO_push(b64, bio);
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);

    BIO_write(bio, data.data(), data.size());
    BIO_flush(bio);
    BIO_get_mem_ptr(bio, &bufferPtr);

    std::string encoded(bufferPtr->data, bufferPtr->length);
    BIO_free_all(bio);

    std::replace(encoded.begin(), encoded.end(), '+', '-');
    std::replace(encoded.begin(), encoded.end(), '/', '_');
    encoded.erase(std::remove(encoded.begin(), encoded.end(), '='), encoded.end());

    return encoded;
}

std::string generate_code_verifier(size_t length) {
    std::vector<unsigned char> buffer(length);
    if (RAND_bytes(buffer.data(), length) != 1) {
        throw std::runtime_error("Failed to generate random bytes for PKCE verifier");
    }
    return base64_url_encode(buffer);
}

std::string generate_code_challenge(const std::string& verifier) {
    std::vector<unsigned char> hash(SHA256_DIGEST_LENGTH);
    SHA256(
        reinterpret_cast<const unsigned char*>(verifier.c_str()),
        verifier.length(),
        hash.data()
    );
    return base64_url_encode(hash);
}

} // namespace core::util
=======
	BIO *bio, *b64;
	BUF_MEM* bufferPtr;

	b64 = BIO_new(BIO_f_base64());
	bio = BIO_new(BIO_s_mem());
	bio = BIO_push(b64, bio);
	BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);

	BIO_write(bio, data.data(), data.size());
	BIO_flush(bio);
	BIO_get_mem_ptr(bio, &bufferPtr);

	std::string encoded(bufferPtr->data, bufferPtr->length);
	BIO_free_all(bio);

	std::replace(encoded.begin(), encoded.end(), '+', '-');
	std::replace(encoded.begin(), encoded.end(), '/', '_');
	encoded.erase(std::remove(encoded.begin(), encoded.end(), '='),
		      encoded.end());

	return encoded;
}

std::string generate_code_verifier(size_t length) {
	std::vector<unsigned char> buffer(length);
	if (RAND_bytes(buffer.data(), length) != 1) {
		throw std::runtime_error(
		    "Failed to generate random bytes for PKCE verifier");
	}
	return base64_url_encode(buffer);
}

std::string generate_code_challenge(const std::string& verifier) {
	std::vector<unsigned char> hash(SHA256_DIGEST_LENGTH);
	SHA256(reinterpret_cast<const unsigned char*>(verifier.c_str()),
	       verifier.length(), hash.data());
	return base64_url_encode(hash);
}

} // namespace Core::Util
>>>>>>> 967ffe1 (core/utils: (feat)Add PKCE implementation for Spotify OAuth)
