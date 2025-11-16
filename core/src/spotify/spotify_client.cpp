#include "cpr/api.h"
#include "cpr/cprtypes.h"
#include "cpr/response.h"
#include "nlohmann/json.hpp"
#include "nlohmann/json_fwd.hpp"
#include "spotify/spotify_client.h"
#include "utils/load_file.h"
#include <cpr/cpr.h>
#include <fstream>
#include <iostream>
#include <print>
#include <string>
#include <vector>

namespace Core {

const std::string TOKEN_URL = "https://accounts.spotify.com/api/token";
const std::string TOKEN_FILE = "tokens.json";

std::string SpotifyClient::exchange_code_for_token(const std::string& code,
						   const std::string& code_verifier,
						   const std::string& redirect_uri) {
	cpr::Payload payload = {
	    {"grant_type", "authorization_code"}, {"code", code},
	    {"redirect_uri", redirect_uri},	  {"client_id", m_client_id},
	    {"client_secret", m_client_secret},	  {"code_verifier", code_verifier}};

	cpr::Response r = cpr::Post(cpr::Url{TOKEN_URL}, payload);

	if (r.status_code == 200) {
		return r.text;
	} else {
		std::cerr << "Error exchanging token: " << r.status_code << std::endl;
		std::cerr << "Response: " << r.text << std::endl;
		return "";
	}
}

std::vector<Playlist_t> SpotifyClient::get_my_playlists() {
	cpr::Header   auth_header = {{"Authorization", "Bearer " + m_token.access_token}};
	cpr::Response r =
	    cpr::Get(cpr::Url{"https://api.spotify.com/v1/me/playlists"}, auth_header);

	if (r.status_code != 200) {
		std::cerr << "Error getting playlists: " << r.status_code << std::endl;
		std::cerr << "Response: " << r.text << std::endl;
		return {};
	}

	std::vector<Playlist_t> playlists;
	try {
		nlohmann::json json_data = nlohmann::json::parse(r.text);

		for (const auto& item : json_data["items"]) {
			Playlist_t playlist;
			playlist.id = item["id"];
			playlist.name = item["name"];
			playlist.owner = item["owner"]["display_name"];
			playlist.collaborative = item["collaborative"];
			playlist.total_tracks = item["tracks"]["total"];

			if (!item["images"].empty()) {
				playlist.image_url = item["images"][0]["url"];
			} else {
				playlist.image_url = "";
			}

			playlists.push_back(playlist);
		}
	} catch (const nlohmann::json::exception& e) {
		std::cerr << "JSON parsing error: " << e.what() << std::endl;
		return {};
	}

	return playlists;
}

std::vector<Track_t> SpotifyClient::get_tracks_from_playlist(const std::string& playlist) {
	if (playlist.empty()) {
		std::println("Invalid Playlist");
		return {};
	}

	cpr::Header auth_header = {{"Authorization", "Bearer " + m_token.access_token}};

	std::vector<Track_t> tracks;
	const int	     limit = 50;
	int		     offset = 0;
	bool		     has_more = true;

	std::string fields =
	    "items(track(id,name,artists(id,name,uri),album(id,name,album_type,images))),total";

	try {
		while (has_more) {
			std::string url = "https://api.spotify.com/v1/playlists/" + playlist +
					  "/tracks?limit=" + std::to_string(limit) +
					  "&offset=" + std::to_string(offset) + "&fields=" + fields;

			cpr::Response r = cpr::Get(cpr::Url{url}, auth_header);

			if (r.status_code != 200) {
				std::println("Failed to fetch playlist: {}", r.status_code);
				break;
			}

			nlohmann::json json_data = nlohmann::json::parse(r.text);

			const auto& items = json_data["items"];
			for (const auto& item : items) {
				if (item["track"].is_null()) {
					continue;
				}

				const auto& track_json = item["track"];
				Track_t	    track;

				track.id = track_json["id"];
				track.name = track_json["name"];

				for (const auto& artist_json : track_json["artists"]) {
					Artist_t artist;
					artist.id = artist_json["id"];
					artist.name = artist_json["name"];
					artist.uri = artist_json["uri"];
					track.artists.push_back(artist);
				}

				const auto& album_json = track_json["album"];
				track.album.id = album_json["id"];
				track.album.name = album_json["name"];
				track.album.type = album_json["album_type"];

				if (!album_json["images"].empty()) {
					const auto& image = album_json["images"][0];
					track.album.album_cover.url = image["url"];
					track.album.album_cover.width =
					    image["width"].is_null()
						? 0
						: image["width"].get<uint32_t>();
					track.album.album_cover.height =
					    image["height"].is_null()
						? 0
						: image["height"].get<uint32_t>();
				} else {
					track.album.album_cover.url = "";
					track.album.album_cover.width = 0;
					track.album.album_cover.height = 0;
				}

				tracks.push_back(track);
			}

			has_more = (items.size() == limit);
			offset += limit;
		}
	} catch (const nlohmann::json::exception& e) {
		std::println("JSON parsing error: {}", e.what());
	}

	return tracks;
}

void SpotifyClient::save_tokens(const std::string& json_response) {
	if (json_response.empty()) {
		std::println(stderr, "[ERROR] Cannot save tokens: empty response");
		return;
	}

	try {
		auto json = nlohmann::json::parse(json_response);

		if (!json.contains("access_token") || !json.contains("token_type")) {
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
			std::println(stderr, "[ERROR] Received empty access token");
		}

	} catch (const nlohmann::json::exception& e) {
		std::println(stderr, "[ERROR] Failed to parse token response: {}", e.what());
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
			std::println(stderr, "[ERROR] Failed to open token file: {}",
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

		if (m_token.access_token.empty() || m_token.refresh_token.empty() ||
		    m_token.token_type.empty() || m_token.scope.empty() || m_token.timestamp == 0 ||
		    m_token.expires_in <= 0) {
			std::println(stderr, "[WARN] Token file is empty or "
					     "malformed. Re-login required.");
			std::filesystem::remove(TOKEN_FILE);
			return false;
		}

		auto now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
		if (now > m_token.timestamp + m_token.expires_in) {
			std::println("[INFO] Access token expired, refreshing...");
			if (refresh_tokens()) {
				mark_tokens_modified();
				save_tokens_to_file();
				return true;
			} else {
				std::println(stderr, "[ERROR] Failed to refresh token. "
						     "Re-login required.");
				std::filesystem::remove(TOKEN_FILE);
				return false;
			}
		}

		std::println("[INFO] Tokens loaded successfully.");
		return true;

	} catch (const nlohmann::json::exception& e) {
		std::println(stderr, "[ERROR] JSON parsing failed: {}", e.what());
		std::filesystem::remove(TOKEN_FILE);
		return false;
	} catch (const std::exception& e) {
		std::println(stderr, "[ERROR] Exception loading tokens: {}", e.what());
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
