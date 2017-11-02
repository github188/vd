/**
 * @file park_lock_control.cpp
 * @brief the common functions for park lock control
 * @author Felix Du <durd07@gmail.com>
 * @version 0.0.1
 * @date 2017-05-22
 */
#include <stdlib.h>
#include <cstring>
#include <string>
#include <fstream>
#include <stdexcept>
#include "json/json.h"

#include "logger/log.h"
#include "park_lock_control.h"
#include "park_lock_cram.h"

using namespace std;

park_lock_operation_t *g_park_lock_operation = NULL;

/* vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv configuration vvvvvvvvvvvvvvvvvvvvvvvvv */

park_lock_configuration_t park_lock_configuration;

static int get_park_lock_configuration(void)
{
    const char* filename = "/mnt/nand/park.conf";

	Json::Reader reader;
	Json::Value root;

	ifstream is;
	is.open(filename, ios::binary);
	if(!reader.parse(is, root))
	{
		is.close();
		ERROR("Parse Json failed. use the default configuration");
        string default_config =
            "{\n\
               \"global\" : {\n\
                    \"city\" : \"青岛\",\n\
                    \"province\" : \"山东\"\n\
                },\n\
               \"device\": {\n\
                    \"parklock\": {\n\
                        \"enable\": 0,\n\
                        \"type\": 2,\n\
                        \"attribute\": {\n\
                            \"gw_ip\": \"192.168.8.8\",\n\
                            \"gw_port\": 8008,\n\
                            \"mac\": \"AABBCCDDEEFFGG\"\n\
                        }\n\
                    }\n\
               }\n\
            }";
        std::ofstream ofs;
        ofs.open(filename);
        ofs << default_config;
        ofs.close();
		return -1;
	}

    try {
        Json::Value park_lock = root["device"]["parklock"];
        park_lock_configuration.enable = park_lock["enable"].asBool();
        park_lock_configuration.type = park_lock["type"].asInt();

        if (park_lock_configuration.type == 2) {
            strcpy(park_lock_configuration.attribute.cram_attribute.gw_ip,
                   park_lock["attribute"]["gw_ip"].asString().c_str());
            strcpy(park_lock_configuration.attribute.cram_attribute.mac,
                   park_lock["attribute"]["mac"].asString().c_str());
            park_lock_configuration.attribute.cram_attribute.gw_port =
                   park_lock["attribute"]["gw_port"].asInt();
        }

        if (park_lock_configuration.enable)
            INFO("park_lock_configuration.enable");

        INFO("gw_ip %s gw_port %d mac %s",
              park_lock_configuration.attribute.cram_attribute.gw_ip,
              park_lock_configuration.attribute.cram_attribute.gw_port,
              park_lock_configuration.attribute.cram_attribute.mac);
    } catch (exception & e) {
        ERROR("%s", e.what());
        is.close();
        return -1;
    }

    is.close();
    return 0;
}

/* ^^^^^^^^^^^^^^^^^^^^^^^^configuration^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^ */

extern int park_lock_onboard_init(void);
void park_lock_init(void)
{
    get_park_lock_configuration();

    int type = park_lock_get_type();
    bool enable = park_lock_enable();

    if (!enable)
        return;

    switch (type) {
    case 1: /* onboard remote control lock */
        park_lock_onboard_init();
        break;
    case 2: /* network remote control lock */ {
        const char* gw_ip = park_lock_cram_gw_ip();
        int port = park_lock_cram_port();
        const char* mac = park_lock_cram_mac();

        park_lock_cram_init(gw_ip, port, mac);
        break;
    }
    default:
        break;
    }
}
