#include "httplib.h"
#include "spotify/secrets.h"
#include "spotify/spotify_client.h"
#include "utils/pkce.h"
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <iostream>
#include <memory>
#include <print>
#include <string>
#include <thread>

std::string g_auth_code;
bool	    g_login_complete = false;

void perform_login_flow(Core::SpotifyClient& client) {
	std::string code_verifier = Core::Util::generate_code_verifier();
	std::string code_challenge = Core::Util::generate_code_challenge(code_verifier);
	std::string auth_url = "https://accounts.spotify.com/authorize?";
	auth_url += "response_type=code";
	auth_url += "&client_id=" + Secret::CLIENT_ID;
	auth_url += "&scope=playlist-read-private";
	auth_url += "&redirect_uri=" + Secret::REDIRECT_URI;
	auth_url += "&code_challenge_method=S256";
	auth_url += "&code_challenge=" + code_challenge;

	httplib::Server server;
	server.Get("/callback", [&](const httplib::Request& req, httplib::Response& res) {
		if (req.has_param("code")) {
			g_auth_code = req.get_param_value("code");
			g_login_complete = true;
			res.set_content("<h1>Success!</h1><p>You can close "
					"this window now.</p>",
					"text/html");
		} else {
			g_login_complete = true;
			res.set_content("<h1>Error</h1><p>No code found in callback.</p>",
					"text/html");
		}
		server.stop();
	});

	std::thread server_thread([&]() {
		if (!server.listen("127.0.0.1", 8888)) {
			std::cerr << "[ERROR] Could not start server on "
				     "127.0.0.1:8888"
				  << std::endl;
			g_login_complete = true;
		}
	});
	server_thread.detach();

	std::cout << "====================================================" << std::endl;
	std::cout << "Please open this URL in your browser to log in:\n" << std::endl;
	std::cout << auth_url << std::endl;
	std::cout << "====================================================" << std::endl;
	std::cout << "\nWaiting for login..." << std::endl;

	while (!g_login_complete) {
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
	}

	if (g_auth_code.empty()) {
		throw std::runtime_error("Login failed. No auth code received.");
	}

	std::cout << "[INFO] Login complete. Exchanging token..." << std::endl;
	std::string json_response =
	    client.exchange_code_for_token(g_auth_code, code_verifier, Secret::REDIRECT_URI);

	if (json_response.empty()) {
		throw std::runtime_error("Failed to exchange token.");
	}

	client.save_tokens(json_response);
	std::cout << "[INFO] Tokens saved successfully." << std::endl;
}

int main(void) {
	try {
		const std::string tokenFile = "tokens.json";
		auto		  client =
		    std::make_unique<Core::SpotifyClient>(Secret::CLIENT_ID, Secret::CLIENT_SECRET);

		if (!std::filesystem::exists(tokenFile) || !client->has_valid_tokens()) {
			std::cout << "[INFO] No valid tokens found. Starting "
				     "login flow.\n";
			perform_login_flow(*client);
		} else {
			std::cout << "[INFO] Found saved tokens. Skipping login.\n";
		}

		// Test get_my_playlists
		std::cout << "\n[INFO] Fetching playlists..." << std::endl;
		auto playlists = client->get_my_playlists();

		std::cout << "\n===== YOUR PLAYLISTS =====" << std::endl;
		for (size_t i = 0; i < playlists.size(); ++i) {
			const auto& pl = playlists[i];
			std::println("{}. {} (by {})", i + 1, pl.name, pl.owner);
			std::println("   ID: {}", pl.id);
			std::println("   Tracks: {}", pl.total_tracks);
			std::println("   Collaborative: {}", pl.collaborative ? "Yes" : "No");
			if (!pl.image_url.empty()) {
				std::println("   Image: {}", pl.image_url);
			}
			std::println("");
		}

		// Test get_tracks_from_playlist on first playlist
		if (!playlists.empty()) {
			const auto& first_playlist = playlists[0];
			std::cout << "\n[INFO] Fetching tracks from: " << first_playlist.name
				  << std::endl;

			auto tracks = client->get_tracks_from_playlist(first_playlist.id);

			std::cout << "\n===== TRACKS IN PLAYLIST =====" << std::endl;
			for (size_t i = 0; i < tracks.size(); ++i) {
				const auto& track = tracks[i];

				// Format artists
				std::string artists_str;
				for (size_t j = 0; j < track.artists.size(); ++j) {
					artists_str += track.artists[j].name;
					if (j < track.artists.size() - 1) {
						artists_str += ", ";
					}
				}

				std::println("{}. {} - {}", i + 1, artists_str, track.name);
				std::println("   Album: {}", track.album.name);

				// Display album cover info
				if (!track.album.album_cover.url.empty()) {
					std::println("   Cover URL: {}",
						     track.album.album_cover.url);
					if (track.album.album_cover.width > 0) {
						std::println("   Cover Size: {}x{}",
							     track.album.album_cover.width,
							     track.album.album_cover.height);
					}
				} else {
					std::println("   Cover: Not available");
				}
				std::println("");
			}

			std::println("[INFO] Total tracks fetched: {}", tracks.size());
		}

		std::cout << "\n[INFO] All done. You are now authenticated." << std::endl;
		return 0;
	} catch (const std::exception& e) {
		std::cerr << "[ERROR] " << e.what() << std::endl;
		return 1;
	}
}
