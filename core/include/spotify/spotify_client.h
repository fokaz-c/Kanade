#pragma once

#include <string>
namespace Core {

class SpotifyClient {
	public:
	SpotifyClient() = default;

    	std::string exchange_code_for_token(
        	const std::string& code,
         	const std::string& code_verifier,
          	const std::string& client_id,
           	const std::string& client_secret,
            	const std::string& redirect_uri
     	);

      	std::string get_my_playlists(const std::string& access_token);

       	void save_tokens(const std::string& json_response);
};

}
