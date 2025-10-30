#include "nlohmann/json.hpp"
#include "nlohmann/json_fwd.hpp"
#include "spotify/spotify_client.h"
#include "utils/load_file.h"
#include <cpr/cpr.h>
#include <fstream>
#include <iostream>
#include <print>

namespace Core {

const std::string TOKEN_URL = "https://accounts.spotify.com/api/token";
const std::string TOKEN_FILE = "tokens.json";

std::string
SpotifyClient::exchange_code_for_token(const std::string& code,
				       const std::string& code_verifier,
				       const std::string& redirect_uri) {
	cpr::Payload payload = {{"grant_type", "authorization_code"},
				{"code", code},
				{"redirect_uri", redirect_uri},
				{"client_id", m_client_id},
				{"client_secret", m_client_secret},
				{"code_verifier", code_verifier}};

	cpr::Response r = cpr::Post(cpr::Url{TOKEN_URL}, payload);

	if (r.status_code == 200) {
		return r.text;
	} else {
		std::cerr << "Error exchanging token: " << r.status_code
			  << std::endl;
		std::cerr << "Response: " << r.text << std::endl;
		return "";
	}
}

std::string SpotifyClient::get_my_playlists() {
	cpr::Header auth_header = {
	    {"Authorization", "Bearer " + m_token.access_token}};
	cpr::Response r = cpr::Get(
	    cpr::Url{"https://api.spotify.com/v1/me/playlists"}, auth_header);

	if (r.status_code == 200) {
		return r.text;
	} else {
		std::cerr << "Error getting playlists: " << r.status_code
			  << std::endl;
		std::cerr << "Response: " << r.text << std::endl;
		return "";
	}
}

void SpotifyClient::save_tokens(const std::string& json_response) {
	if (json_response.empty()) {
		std::println(stderr,
			     "[ERROR] Cannot save tokens: empty response");
		return;
	}

	try {
		auto json = nlohmann::json::parse(json_response);

		// Validate required fields exist
		if (!json.contains("access_token") ||
		    !json.contains("token_type")) {
			std::println(stderr, "[ERROR] Invalid token response: "
					     "missing required fields");
			return;
		}

		m_token.access_token = json.value("access_token", "");
		m_token.expires_in = json.value("expires_in", 0);
		m_token.refresh_token = json.value("refresh_token", "");
		m_token.scope = json.value("scope", "");
		m_token.token_type = json.value("token_type", "");

		auto now = std::chrono::system_clock::now();
		m_token.timestamp = std::chrono::system_clock::to_time_t(now);

		// Only mark modified and save if we got valid tokens
		if (!m_token.access_token.empty()) {
			mark_tokens_modified();
			save_tokens_to_file();
		} else {
			std::println(stderr,
				     "[ERROR] Received empty access token");
		}

	} catch (const nlohmann::json::exception& e) {
		std::println(stderr,
			     "[ERROR] Failed to parse token response: {}",
			     e.what());
	}
}

void SpotifyClient::save_tokens_to_file() {
	nlohmann::json json;
	json["access_token"] = m_token.access_token;
	json["expires_in"] = m_token.expires_in;
	json["refresh_token"] = m_token.refresh_token;
	json["scope"] = m_token.scope;
	json["token_type"] = m_token.token_type;
	json["timestamp"] = m_token.timestamp;

	std::ofstream file(TOKEN_FILE);
	if (file.is_open()) {
		file << json.dump(4);
		file.close();
		std::println("Tokens saved to {}", TOKEN_FILE);
		m_tokens_modified = false; // Reset flag after successful save
	} else {
		std::println(stderr, "Error: Could not write {}", TOKEN_FILE);
	}
}

bool SpotifyClient::load_tokens() {
	try {
		if (!std::filesystem::exists(TOKEN_FILE)) {
			std::println(stderr, "[WARN] Token file not found.");
			return false;
		}

		auto result = Core::Util::MappedFile::open(TOKEN_FILE.c_str());
		if (!result) {
			std::println(stderr,
				     "[ERROR] Failed to open token file: {}",
				     result.error().message());
			return false;
		}

		auto& file = result.value();
		auto  j = nlohmann::json::parse(file.to_string());

		m_token.access_token = j.value("access_token", "");
		m_token.expires_in = j.value("expires_in", 0);
		m_token.refresh_token = j.value("refresh_token", "");
		m_token.scope = j.value("scope", "");
		m_token.token_type = j.value("token_type", "");
		m_token.timestamp = j.value("timestamp", 0L);

		if (m_token.access_token.empty() ||
		    m_token.refresh_token.empty() ||
		    m_token.token_type.empty() || m_token.scope.empty() ||
		    m_token.timestamp == 0 || m_token.expires_in <= 0) {
			std::println(stderr, "[WARN] Token file is empty or "
					     "malformed. Re-login required.");
			std::filesystem::remove(TOKEN_FILE);
			return false;
		}

		auto now = std::chrono::system_clock::to_time_t(
		    std::chrono::system_clock::now());
		if (now > m_token.timestamp + m_token.expires_in) {
			std::println(
			    "[INFO] Access token expired, refreshing...");
			if (refresh_tokens()) {
				mark_tokens_modified();
				save_tokens_to_file();
				return true;
			} else {
				std::println(stderr,
					     "[ERROR] Failed to refresh token. "
					     "Re-login required.");
				std::filesystem::remove(TOKEN_FILE);
				return false;
			}
		}

		std::println("[INFO] Tokens loaded successfully.");
		return true;

	} catch (const nlohmann::json::exception& e) {
		std::println(stderr, "[ERROR] JSON parsing failed: {}",
			     e.what());
		std::filesystem::remove(TOKEN_FILE);
		return false;
	} catch (const std::exception& e) {
		std::println(stderr, "[ERROR] Exception loading tokens: {}",
			     e.what());
		std::filesystem::remove(TOKEN_FILE);
		return false;
	}
}

bool SpotifyClient::refresh_tokens() {
	cpr::Payload payload = {{"grant_type", "refresh_token"},
				{"refresh_token", m_token.refresh_token},
				{"client_id", m_client_id},
				{"client_secret", m_client_secret}};

	cpr::Response r = cpr::Post(cpr::Url{TOKEN_URL}, payload);

	if (r.status_code != 200) {
		std::println(stderr, "Token refresh failed: {}", r.status_code);
		std::println(stderr, "Response: {}", r.text);
		return false;
	}

	auto json = nlohmann::json::parse(r.text);

	m_token.access_token = json.value("access_token", "");
	m_token.expires_in = json.value("expires_in", m_token.expires_in);

	auto now = std::chrono::system_clock::now();
	m_token.timestamp = std::chrono::system_clock::to_time_t(now);

	mark_tokens_modified();
	std::println("Access token refreshed successfully.");
	return true;
}

} // namespace Core
