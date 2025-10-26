#include "nlohmann/json.hpp"
#include "spotify/spotify_client.h"
#include <cpr/cpr.h>
#include <fstream>
#include <iostream>

namespace Core {

const std::string TOKEN_URL = "https://accounts.spotify.com/api/token";
const std::string TOKEN_FILE = "tokens.json";

std::string SpotifyClient::exchange_code_for_token(
    const std::string& code, const std::string& code_verifier,
    const std::string& client_id, const std::string& client_secret,
    const std::string& redirect_uri) {
	cpr::Payload payload = {{"grant_type", "authorization_code"},
				{"code", code},
				{"redirect_uri", redirect_uri},
				{"client_id", client_id},
				{"client_secret", client_secret},
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

std::string SpotifyClient::get_my_playlists(const std::string& access_token) {
	cpr::Header auth_header = {{"Authorization", "Bearer " + access_token}};

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
	try {
		auto json = nlohmann::json::parse(json_response);

		std::ofstream file(TOKEN_FILE);
		if (!file.is_open()) {
			std::cerr << "Error: Could not open " << TOKEN_FILE
				  << " for writing" << std::endl;
			return;
		}

		file << json.dump(4);
		file.close();

		std::cout << "Tokens saved to " << TOKEN_FILE << std::endl;
	} catch (const nlohmann::json::exception& e) {
		std::cerr << "Error parsing JSON: " << e.what() << std::endl;
	}
}

} // namespace Core
