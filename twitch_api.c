#include "twitch_api.h"

#define TWITCH_API_CLIENT_ID_HEADER "Client-ID: rqm7hvz8t4im4isn53zogkmi1pvzd5p"

#define REQ_URL_BUF_SIZE 512
#define BASE_REQUEST_URL "https://api.twitch.tv/helix/streams?user_login="

static char req_buf[REQ_URL_BUF_SIZE] = {0};

struct curl_write_stat
{
	char *memory;
	size_t size;
};

static size_t curl_write(char *contents, size_t size, size_t nmemb, void *userdata)
{
	size_t realsize = size * nmemb;
	struct curl_write_stat *mem = (struct curl_write_stat *)userdata;

	char *ptr = realloc(mem->memory, mem->size + realsize + 1);
	if (ptr == NULL)
	{
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
static CURL *curl = NULL;
static struct curl_write_stat curl_get_data = {0};

static size_t get_stream_info_json(const char *url)
{
	curl_get_data.size = 0;

	if (curl == NULL)
	{
		curl_global_init(CURL_GLOBAL_DEFAULT);

		curl = curl_easy_init();
	}
	
	if (curl == NULL)
	{
		return -1;
	}

	struct curl_slist *header_chunk = NULL;

	header_chunk = curl_slist_append(header_chunk, TWITCH_API_CLIENT_ID_HEADER);
	curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header_chunk);
	curl_easy_setopt(curl, CURLOPT_URL, url);

	curl_get_data.memory = malloc(1);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
	curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&curl_get_data);

	const CURLcode res = curl_easy_perform(curl);
	/* Check for errors */
	if (res != CURLE_OK)
	{
		fprintf(stderr, "curl_easy_perform() failed: %s\n",
				curl_easy_strerror(res));
	}

	/* always cleanup */
	curl_easy_cleanup(curl);

	return curl_get_data.size;
}

static bool build_request_url(struct stream_trigger_t* triggers, char *buf)
{
	/* twitch api makes you repeat the same parameter which is kinda jank */
	/* ?user_login=<1>&user_login=<2> .. */
	memset(buf, 0, REQ_URL_BUF_SIZE);
	strcpy_s(buf, REQ_URL_BUF_SIZE, BASE_REQUEST_URL);

	char user_chunk[64] = { 0 };
	bool first = true;
	bool got_any = false;
	
	for (size_t i = 0; i < NUM_HARDCODED_TRIGGERS; ++i)
	{
		struct stream_trigger_t *trigger = triggers + i;
		if (!trigger->enabled || trigger->account[0] == '\0')
		{
			continue;
		}
		
		got_any = true;
		const char *format = first ? "%s" : "&user_login=%s";
		first = false;
		size_t written = snprintf(user_chunk, 64, format, trigger->account);
		/* ouch: second param is size of DEST buffer max - not our written chunksz */
		strcat_s(buf, REQ_URL_BUF_SIZE, user_chunk);
	}

	return got_any;
}

void parse_json(const char *json_str)
{
	cJSON *json = cJSON_Parse(json_str);
	if (json == NULL)
	{
		const char *error_ptr = cJSON_GetErrorPtr();
		if (error_ptr != NULL)
		{
			fprintf(stderr, "json parsing failed, error before: %s\n", error_ptr);
		}
	}
	else
	{			
		struct cJSON* data_arr = cJSON_GetObjectItem(json, "data");
		if (data_arr)
		{
			// ReSharper disable CppJoinDeclarationAndAssignment
			struct cJSON* twitch_account;

			cJSON_ArrayForEach(twitch_account, data_arr)
			{
				cJSON* user_name = cJSON_GetObjectItem(twitch_account, "user_name");
				/* dont bother checking type here, if its in list its live in some capacity */
				trigger_user_online(user_name->valuestring);
			}
			
			// ReSharper enable CppJoinDeclarationAndAssignment
		}
	}
}

void get_streams_status(struct stream_trigger_t *triggers)
{
	if (build_request_url(triggers, req_buf))
	{
		size_t json_size = get_stream_info_json(req_buf);
		parse_json(curl_get_data.memory);
	}
}
