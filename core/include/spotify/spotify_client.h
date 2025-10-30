#pragma once
#include <chrono>
#include <cstdint>
#include <string>

namespace Core {

struct token_t {
	std::string access_token;
	int	    expires_in;
	std::string refresh_token;
	std::string scope;
	std::string token_type;
	int64_t	    timestamp;
};

class SpotifyClient {
      public:
	SpotifyClient(std::string clientID, std::string clientSecret)
	    : m_client_id(std::move(clientID)),
	      m_client_secret(std::move(clientSecret)),
	      m_tokens_modified(false) {
		load_tokens();
	}

	~SpotifyClient() {
		if (m_tokens_modified && !m_token.access_token.empty()) {
			save_tokens_to_file();
		}
	}

	// Prevent copying (tokens should have unique ownership)
	SpotifyClient(const SpotifyClient&) = delete;
	SpotifyClient& operator=(const SpotifyClient&) = delete;

	// Allow moving
	SpotifyClient(SpotifyClient&&) = default;
	SpotifyClient& operator=(SpotifyClient&&) = default;

	bool has_valid_tokens() const {
		if (m_token.access_token.empty() || m_token.expires_in <= 0 ||
		    m_token.timestamp == 0)
			return false;

		auto now = std::chrono::system_clock::now();
		auto token_time = std::chrono::system_clock::time_point(
		    std::chrono::seconds(m_token.timestamp));
		auto expiry_time =
		    token_time + std::chrono::seconds(m_token.expires_in);

		return now < expiry_time;
	}

	std::string exchange_code_for_token(const std::string& code,
					    const std::string& code_verifier,
					    const std::string& redirect_uri);
	std::string get_my_playlists();

	// Public API: parses JSON response and updates tokens
	void save_tokens(const std::string& json_response);

	bool refresh_tokens();

      private:
	bool load_tokens();

	void save_tokens_to_file();

	void mark_tokens_modified() {
		m_tokens_modified = true;
	}

	std::string api_get(const std::string& endpoint);

      private:
	token_t	    m_token;
	std::string m_client_id;
	std::string m_client_secret;
	bool	    m_tokens_modified;
};

} // namespace Core
