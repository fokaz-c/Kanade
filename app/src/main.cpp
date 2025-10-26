#include "httplib.h"
#include "spotify/secrets.h"
#include "spotify/spotify_client.h"
#include "utils/pkce.h"
#include <chrono>
#include <iostream>
#include <string>
#include <thread>

std::string g_auth_code;
bool g_login_complete = false;

int main(void) {
	std::string code_verifier = Core::Util::generate_code_verifier();
	std::string code_challenge =
	    Core::Util::generate_code_challenge(code_verifier);

	std::string auth_url = "https://accounts.spotify.com/authorize?";
	auth_url += "response_type=code";
	auth_url += "&client_id=" + Secret::CLIENT_ID;
	auth_url += "&scope=playlist-read-private";
	auth_url += "&redirect_uri=" + Secret::REDIRECT_URI;
	auth_url += "&code_challenge_method=S256";
	auth_url += "&code_challenge=" + code_challenge;

	httplib::Server server;
	server.Get("/callback", [&](const httplib::Request& req,
				    httplib::Response& res) {
		if (req.has_param("code")) {
			g_auth_code = req.get_param_value("code");
			g_login_complete = true;
			res.set_content("<h1>Success!</h1><p>You can close "
					"this window now.</p>",
					"text/html");
		} else {
			g_login_complete = true;
			res.set_content(
			    "<h1>Error</h1><p>No code found in callback.</p>",
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

	std::cout << "===================================================="
		  << std::endl;
	std::cout << "Please open this URL in your browser to log in:\n"
		  << std::endl;
	std::cout << auth_url << std::endl;
	std::cout << "===================================================="
		  << std::endl;
	std::cout << "\nWaiting for login..." << std::endl;

	while (!g_login_complete) {
		std::this_thread::sleep_for(std::chrono::milliseconds(200));
	}

	if (g_auth_code.empty()) {
		std::cerr << "[ERROR] Login failed. No auth code received."
			  << std::endl;
		return 1;
	}

	std::cout << "[INFO] Login complete. Exchanging token..." << std::endl;

	Core::SpotifyClient client;
	std::string json_response = client.exchange_code_for_token(
	    g_auth_code, code_verifier, Secret::CLIENT_ID,
	    Secret::CLIENT_SECRET, Secret::REDIRECT_URI);

	if (json_response.empty()) {
		std::cerr << "[ERROR] Failed to exchange token." << std::endl;
		return 1;
	}

	client.save_tokens(json_response);

	std::cout << "[INFO] All done. You are now authenticated." << std::endl;

	return 0;
}
