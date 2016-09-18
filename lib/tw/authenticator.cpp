#include <algorithm>
#include <ctime>
#include <fstream>
#include <iostream>
#include <sstream>
#include <openssl/hmac.h>
#include <tw/authenticator.h>
#include <tw/base64.h>
#include <tw/oauth.h>
#include <utils.h>

static const char *INSTRUCTIONS = "In order to allow LynxBot to use your "
"Twitter account, you must register it as a Twitter app. This is done by "
"signing into https://apps.twitter.com and clicking the \"Create New App\" "
"button. Give your app a unique name (e.g. lynxbot-TWITTERNAME), fill in the "
"rest of the form and agree to the Developer Agreement.\n\nOnce you submit the "
"form you will be redirected to your app's overview page. You can change the "
"access level to read only (LynxBot does not do any writing).\nNext, click the "
"Keys and Access Tokens tab. Under \"Your Access Token\", click Create my "
"access token to generate access tokens for LynxBot.\n\nYou will now see four "
"tokens on the page:\nConsumer Key\nConsumer Secret\nAccess Token\nAccess "
"Token Secret\nYou will need to enter them into this console.\n\nWARNING: "
"Your access tokens will be stored in plaintext.\n";

static void get_str(std::string &target);

tw::Authenticator::Authenticator()
{
	configpath = utils::configdir() + utils::config("twitter");
	if (read_keys() == -1)
		interactive_setup();
}

/* siggen: generate an oauth signature for a twitter api request */
void tw::Authenticator::siggen(const std::string &method, const std::string &URL,
		const param_vec &head_params, const param_vec &body_params)
{
	size_t i;
	std::vector<std::string> enc_params;
	std::string base_str, signing_key;
	unsigned char digest[1024];
	unsigned int digest_len;

	/* get ouath data */
	odata.c_key = consumerkey;
	odata.nonce = noncegen();
	odata.sig_method = "HMAC-SHA1";
	odata.timestamp = std::to_string(time(nullptr));
	odata.token = token;
	odata.version = "1.0";

	/* generate the base string */
	for (auto p : head_params)
		enc_params.push_back(pencode(p.first) + "=" + pencode(p.second));
	for (auto p : body_params)
		enc_params.push_back(pencode(p.first) + "=" + pencode(p.second));
	enc_params.push_back("oauth_consumer_key=" + odata.c_key);
	enc_params.push_back("oauth_nonce=" + odata.nonce);
	enc_params.push_back("oauth_signature_method=" + odata.sig_method);
	enc_params.push_back("oauth_timestamp=" + odata.timestamp);
	enc_params.push_back("oauth_token=" + odata.token);
	enc_params.push_back("oauth_version=" + odata.version);

	std::sort(enc_params.begin(), enc_params.end());

	std::string param_str;
	for (i = 0; i < enc_params.size(); ++i) {
		param_str += enc_params[i];
		if (i != enc_params.size() - 1)
			param_str += '&';
	}

	base_str += method + "&";
	base_str += pencode(URL) + "&";
	base_str += pencode(param_str);

	/* get the signing key */
	signing_key += pencode(consumersecret) + "&";
	signing_key += pencode(tokensecret);

	HMAC(EVP_sha1(), signing_key.c_str(), signing_key.length(),
			(unsigned char *)base_str.c_str(), base_str.length(),
			digest, &digest_len);

	odata.sig = base64_enc((char *)digest, digest_len);
}

struct tw::Authenticator::oauth_data tw::Authenticator::auth_data()
{
	return odata;
}

/* read_keys: read twitter keys from file (will be encrypted eventually) */
int tw::Authenticator::read_keys()
{
	std::ifstream reader(configpath);
	std::string line;
	int lnum = 0;

	if (!reader.is_open())
		return -1;
	while (std::getline(reader, line)) {
		switch (++lnum) {
		case 1:
			consumerkey = line;
			break;
		case 2:
			consumersecret = line;
			break;
		case 3:
			token = line;
			break;
		case 4:
			tokensecret = line;
			break;
		default:
			return 1;
		}
	}
	if (lnum != 4)
		return 1;

	return 0;
}

/* interactive_setup: obtain access tokens from user */
void tw::Authenticator::interactive_setup()
{
	char c = '\0';
	std::cout << configpath << " was not found." << std::endl;
	std::cout << "Would you like to set up LynxBot to use Twitter? (y/n) ";
	std::ofstream writer(configpath);

	while (c != 'y' && c != 'n')
		std::cin >> c;
	if (c == 'n')
		return;

	std::cout << INSTRUCTIONS << std::endl;
	std::cout << "Would you like to proceed? (y/n) ";

	c = '\0';
	while (c != 'y' && c != 'n')
		std::cin >> c;
	if (c == 'n')
		return;

	std::cout << "Enter your Consumer Key:" << std::endl;
	get_str(consumerkey);
	std::cout << "Enter your Consumer Secret:" << std::endl;
	get_str(consumersecret);
	std::cout << "Enter your Access Token:" << std::endl;
	get_str(token);
	std::cout << "Enter your Access Token Secret:" << std::endl;
	get_str(tokensecret);

	writer << consumerkey << std::endl << consumersecret << std::endl
		<< token << std::endl << tokensecret << std::endl;
	writer.close();

	std::cout << "Your tokens have been saved to " << configpath
		<< std::endl;
	std::cin.get();
}

/* get_str: read a string from stdin */
static void get_str(std::string &target)
{
	std::string line;
	while (true) {
		std::getline(std::cin, line);
		if (!line.empty()) break;
		std::cout << "Invalid string, try again:" << std::endl;
	}
	target = line;
}
