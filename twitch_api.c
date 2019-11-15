#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

struct curl_write_stat {
	char* memory;
	size_t size;
};

static size_t curl_write(char* contents, size_t size, size_t nmemb, void* userdata)
{
	size_t realsize = size * nmemb;
	struct curl_write_stat* mem = (struct curl_write_stat*)userdata;

	char* ptr = realloc(mem->memory, mem->size + realsize + 1);
	if (ptr == NULL) {
		/* out of memory! */
		printf("not enough memory (realloc returned NULL)\n");
		return 0;
	}

	mem->memory = ptr;
	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}

/* curl_global_cleanup(); */
static CURL* curl = NULL;
struct curl_write_stat get_data = { 0 };

int get_stream_info_json(char *buf, size_t maxsz)
{	
	CURLcode res;

	if (curl == NULL)
	{
		curl_global_init(CURL_GLOBAL_DEFAULT);

		curl = curl_easy_init();
	}
	if (curl == NULL)
	{
		return -1;
	}
	
	struct curl_slist* header_chunk = NULL;

	header_chunk = curl_slist_append(header_chunk, "Client-ID: rqm7hvz8t4im4isn53zogkmi1pvzd5p");
	/* set our custom set of headers */
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_chunk);
	/*
	curl -H 'Client-ID: rqm7hvz8t4im4isn53zogkmi1pvzd5p' -X GET 'https://api.twitch.tv/helix/streams?user_login=sitzkrieg'
	*/
	curl_easy_setopt(curl, CURLOPT_URL, "https://api.twitch.tv/helix/streams?user_login=KingGeorge");
	
	get_data.memory = malloc(1); 
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void*)&get_data);

	res = curl_easy_perform(curl);
	/* Check for errors */
	if (res != CURLE_OK)
	{
		fprintf(stderr, "curl_easy_perform() failed: %s\n",
			curl_easy_strerror(res));
	}
	else
	{
		//const char* json_str = (const char*)get_data.memory;
		//cJSON* json = cJSON_Parse(json_str);
		//printf("`%s`\r\n", cJSON_Print(json));
		/* TODO: copy string out somewhere */
	}
	/* always cleanup */
	curl_easy_cleanup(curl);

	return get_data.size;
}