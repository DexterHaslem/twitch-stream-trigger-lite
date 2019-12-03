#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "trigger.h"
#include "cJSON.h"

void get_streams_status(struct stream_trigger_t *triggers);
