/**
 * @file park_lock_cram.cpp
 * @brief this file include the park lock control functions for park lock
 *        from CrAM IoT Technology Co.,Ltd. including protocal analyze, park
 *        lock control etc.
 * @author Felix Du <durd07@gmail.com>
 * @version 0.0.1
 * @date 2017-05-18
 */

#include "json/json.h"
#include <cstdlib>
#include <cstring>
#include "logger/log.h"

#include "park_lock_cram.h"
#include "park_lock_cram_protocal.h"

extern park_lock_cram_resp_cb resp_cb_table[PARK_LOCK_CRAM_RESP_MAX];
int park_lock_cram_construct_json(const cram_lock_cmd_req *cmd,
                                         char* result, size_t result_len)
{
    if ((NULL == cmd) || (NULL == result)) {
        return -1;
    }

    Json::Value root;
    root["ver"] = cmd->ver;
    root["mac"] = cmd->mac;
    root["code"] = cmd->code;
    root["type"] = cmd->type;
    root["payload"] = cmd->payload;

    Json::FastWriter writer;
    std::string ret = writer.write(root);

    if (ret.size() >= result_len) {
        ERROR("buffer exceed the len %s %d", ret.c_str(), result_len);
        return -1;
    }

    memcpy(result, ret.c_str(), ret.size());
    return 0;
}

int park_lock_cram_decode_data(const char* buff, const size_t len)
{
    Json::Reader reader;
    Json::Value root;

    if (!reader.parse(buff, root)) {
        //ERROR("parse json failed.");
        return -1;
    }

    const char* payload = root["payload"].asString().c_str();

    if (NULL == payload) {
        ERROR("parse payload failed.");
        return -1;
    }

    if (strncmp(payload, "[4", 2) == 0) {
        resp_cb_table[PARK_LOCK_CRAM_RESP_STATE]((void*)payload);
    } else if (strcmp(payload, "[F0:01]") == 0) {
        resp_cb_table[PARK_LOCK_CRAM_RESP_REFRESH_BEGIN]((void*)payload);
    } else if (strcmp(payload, "[F0:02]") == 0) {
        resp_cb_table[PARK_LOCK_CRAM_RESP_REFRESH_FINISH]((void*)payload);
    } else if (strcmp(payload, "[F0:03]") == 0) {
        resp_cb_table[PARK_LOCK_CRAM_RESP_REFRESH_ERROR]((void*)payload);
    } else if (strncmp(payload, "[F1:", 4) == 0) {
        resp_cb_table[PARK_LOCK_CRAM_RESP_LOCK_LIST]((void*)payload);
    }
    return 0;
}
