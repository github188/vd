#include "../../lib/Json/include/json/json.h"
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <fstream>
#include <map>
#include <string>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>

#include "commonfuncs.h"
#include "logger/log.h"

#include "park_file_handler.h"
#include "park_status.h"
#include "park_status_db_api.h"

#if 1 /****************** light handler ******************/
struct light_string_map
{
    const CURRENT_LIGHT_STATE light_state;
    const std::string light_str;
};

struct light_string_map light_string_map_table[7] = {
    {CURRENT_LIGHT_STATE::NOLIGHT,      "全灭"},
    {CURRENT_LIGHT_STATE::GREEN,        "绿灯"},
    {CURRENT_LIGHT_STATE::GREEN_FLASH,  "绿闪"},
    {CURRENT_LIGHT_STATE::RED,          "红灯"},
    {CURRENT_LIGHT_STATE::RED_FLASH,    "红闪"},
    {CURRENT_LIGHT_STATE::BLUE,         "蓝灯"},
    {CURRENT_LIGHT_STATE::BLUE_FLASH,   "蓝闪"}
};

static std::string get_light_str_from_state(const CURRENT_LIGHT_STATE state)
{
#if 0
    for (auto item : light_string_map_table) {
#else
    int s = sizeof(light_string_map_table) / sizeof(light_string_map_table[0]);
    for (int i = 0; i < s; i++) {
        auto item = light_string_map_table[i];
#endif
        if (state == item.light_state) {
            return item.light_str;
        }
    }
    return "";
}

static CURRENT_LIGHT_STATE get_light_from_str(const std::string & light)
{
#if 0
    for (auto item : light_string_map_table) {
#else
    int s = sizeof(light_string_map_table) / sizeof(light_string_map_table[0]);
    for (int i = 0; i < s; i++) {
        auto item = light_string_map_table[i];
#endif
        if (light == item.light_str) {
            return item.light_state;
        }
    }
    return CURRENT_LIGHT_STATE::NOLIGHT;
}
#endif /****************** light handler ******************/

int push_record_2_history_list(const int objectState,
                               const char* plate,
                               const char* park_time,
                               const char* pic_name1,
                               const char* pic_name2,
                               const char* light)
{
    return parkStatus::push_record_2_history_list(objectState, plate,
            park_time, pic_name1, pic_name2, light);
}

parkStatus::parkStatus()
{
    memset(&m_park_status, 0x00, sizeof(struct park_status_t));

    memset(m_history_list_entry, 0x00,
           sizeof(struct park_history_t) * PARK_RECORD_HISTORY_MAX);

    current_history = -1;

    /* open the db in default path PARK_STATE_DB_PATH*/
    if (get_sd_mount_status() == true) {
        park_state_db_open(PARK_STATE_DB_PATH);
        park_state_db_create_tb();
    }
}

parkStatus::~parkStatus()
{
	park_state_db_close();
}

/* this is the callback function when select expired items from db. */
int delete_expired_item_cb(void* data, int argc, char **argv, char **columnName)
{
    for (int i = 0; i < argc; i++) {
        INFO("%s = %s", columnName[i], argv[i] ? argv[i] : "NULL");

        if ((strcmp(columnName[i], "pic_name1") == 0) ||
            (strcmp(columnName[i], "pic_name2") == 0)) {
            const char* pic_name = argv[i] ? argv[i] : NULL;

            if (pic_name == NULL)
                continue;

            char pic_path[128] = {0};
            sprintf(pic_path, "/var/www/parkjson/%s", pic_name);
            park_delete_picture(pic_path);
        }
    }
    return 0;
}


/**
 * @brief return the sd mount status
 *
 * @return true mount ok
 *         false mount fail
 */
bool parkStatus::get_sd_mount_status(void)
{
    int fd = -1;
    if ((fd = open("/proc/mounts", O_RDONLY)) < 0) {
        ERROR("open /proc/mounts failed. errno = %d", errno);
        return false;
    }

    int ret = 0;
    char buffer[1024] = {0};
    if ((ret = read(fd, buffer, sizeof(buffer))) < 0) {
        ERROR("read /proc/mounts failed. errno = %d", errno);
		goto fail;
    }

    if (strstr(buffer, "/mnt/mmc") != NULL) {
		close(fd);
        return true;
    }

fail:
	close(fd);
    return false;
}

int parkStatus::push_record_2_history_list(const int objectState,
                                           const char* plate,
                                           const char* park_time,
                                           const char* pic_name1,
                                           const char* pic_name2,
                                           const char* light)
{
    static parkStatus *instance = NULL;
    if (NULL == instance) {
        instance = new parkStatus();
        instance->load_from_file(PARK_HISTORY_FILE);
    }

    /* this is the light state changed */
    if (light != NULL) {
        instance->m_park_status.currentlightstate = get_light_from_str(light);
        instance->dump_to_file(PARK_HISTORY_FILE);
        return 0;
    }

    if (objectState == 1) { /* drive in */
        instance->m_park_status.parkstate = CURRENT_PARK_STATE::HASCAR;
		convert_enc_s("GBK", "UTF-8", plate, strlen(plate) + 1,
                instance->m_park_status.currentplate, PARK_PLATE_LENGTH);
    } else { /* drive out */
        instance->m_park_status.parkstate = CURRENT_PARK_STATE::NOCAR;
        memset(instance->m_park_status.currentplate, 0x00,
                sizeof(instance->m_park_status.currentplate));
    }

    (instance->current_history)++;
    if (instance->current_history >= PARK_RECORD_HISTORY_MAX) {
        instance->current_history = 0;
    }

	struct park_history_t *new_node =
        &(instance->m_history_list_entry[instance->current_history]);

    /* delete pictures oldder than 90 days. */
    const char* cmd = "find /var/www/parkjson/*.jpg -mtime +90 -exec rm -rf {} \\;";
    system(cmd);
    DEBUG("rm pic cmd = %s", cmd);

    /* detect if SD card is mount */
    bool sd_mount_ok = instance->get_sd_mount_status();
    if (sd_mount_ok == false) {
        /* if there are data in this node already, delete related pictures. */
        if (new_node->index != 0) {
            char pic_path[128] = {0};
            char* pic_name1 = new_node->pic_name[0];
            sprintf(pic_path, "/var/www/parkjson/%s", pic_name1);
            park_delete_picture(pic_path);
            memset(pic_path, 0x00, sizeof(pic_path));
            char* pic_name2 = new_node->pic_name[1];
            sprintf(pic_path, "/var/www/parkjson/%s", pic_name2);
            park_delete_picture(pic_path);
        }
    }

    memset(new_node, 0x00, sizeof(park_history_t));
    new_node->index = 99;
    convert_enc_s("GBK", "UTF-8", plate, strlen(plate) + 1,
            new_node->plate, PARK_PLATE_LENGTH);
    memcpy(new_node->time, park_time, strlen(park_time) + 1);

    if (pic_name1 != NULL)
        memcpy(new_node->pic_name[0], pic_name1, strlen(pic_name1) + 1);
    if (pic_name2 != NULL)
        memcpy(new_node->pic_name[1], pic_name2, strlen(pic_name2) + 1);

    if (objectState == 1) {
        new_node->state = PARK_RECORD_STATE::DRIVE_IN;
    } else {
        new_node->state = PARK_RECORD_STATE::DRIVE_OUT;
    }

    instance->dump_to_file(PARK_HISTORY_FILE);

    if (sd_mount_ok == false) {
        return 0;
    }

    /* store the park history into db, before it, delete the expired items. */
	time_t now;

	time(&now);

    time_t expired_time = now - 24*3600*90; /* 90 days before */

	struct tm *expired;
	expired = localtime(&expired_time);

    char time_format[20] = {0};
    sprintf(time_format, "%04d-%02d-%02d %02d:%02d:%02d",
            expired->tm_year + 1900, expired->tm_mon + 1,
            expired->tm_mday, expired->tm_hour,
            expired->tm_min, expired->tm_sec);

    /* select all the records expired and delete related pictures. */
    park_state_db_select_parkhistory_tb(NULL, time_format, delete_expired_item_cb);
    park_state_db_delete_parkhistory_old(time_format);
    park_state_db_insert_parkhistory_tb(new_node);
    return 0;
}

int parkStatus::dump_to_file(const char* file_name)
{
    if (NULL == file_name) {
        ERROR("NULL is file_name");
        return -1;
    }

    struct park_status_t &park_status = this->m_park_status;

    Json::Value root;
    root["parkstate"] = static_cast<int>(park_status.parkstate);
    root["currentplate"] = park_status.currentplate;
    root["spacecode"] = park_status.spacecode;
    root["parklightstate"] = get_light_str_from_state(park_status.currentlightstate);

    int i = current_history;
    int index = 1;

    do {
        if (i < 0) break;

        Json::Value history_item;
        history_item["index"] = index;
        history_item["time"] = m_history_list_entry[i].time;
        history_item["plate"] = m_history_list_entry[i].plate;
        history_item["pic_name1"] = m_history_list_entry[i].pic_name[0];
        history_item["pic_name2"] = m_history_list_entry[i].pic_name[1];

        if (m_history_list_entry[i].state == PARK_RECORD_STATE::DRIVE_IN) {
            history_item["state"] = "驶入";
        } else {
            history_item["state"] = "驶离";
        }

        root["parkhistory"].append(history_item);

        i--;
        index++;
        if (i < 0) {
            if (m_history_list_entry[PARK_RECORD_HISTORY_MAX - 1].index != 0) {
                i = PARK_RECORD_HISTORY_MAX - 1;
            } else {
                break;
            }
        }
    } while (i != current_history);

	Json::StyledWriter writer;
	std::string strWrite = writer.write(root);
    //DEBUG(strWrite.c_str());
	std::ofstream ofs;
	ofs.open(file_name);
	ofs << strWrite;
	ofs.close();
    return 0;
}

int parkStatus::load_from_file(const char* file_name)
{
    if (NULL == file_name) {
        ERROR("NULL is file_name");
        return -1;
    }

	Json::Reader reader;
	Json::Value root;

    std::fstream fs;
    fs.open(file_name, std::fstream::in);
	if(!reader.parse(fs, root))
	{
		ERROR("Parse Json failed.");
		fs.close();
		return -1;
	}

    parse_from_json(root);
    fs.close();
    return 0;
}

int parkStatus::parse_from_json(Json::Value &root)
{
    struct park_status_t &park_status = this->m_park_status;

    const int hascar = static_cast<int>(CURRENT_PARK_STATE::HASCAR);
    if (root["parkstate"].asInt() == hascar) {
        park_status.parkstate = CURRENT_PARK_STATE::HASCAR;
    } else {
        park_status.parkstate = CURRENT_PARK_STATE::NOCAR;
    }

    extern ARM_config g_arm_config; //arm参数结构体全局变量
    const char* spacecode = g_arm_config.basic_param.spot_id; //root["spacecode"].asString().c_str();
    const size_t spacecode_length = strlen(spacecode) < SPACE_CODE_LENGTH ?
                          strlen(spacecode) : SPACE_CODE_LENGTH;
    strncpy(park_status.spacecode, spacecode, spacecode_length);
    park_status.spacecode[spacecode_length] = '\0';

    const char* plate = root["currentplate"].asString().c_str();
    const size_t plate_length = strlen(plate) < PARK_PLATE_LENGTH ?
                          strlen(plate) : PARK_PLATE_LENGTH;
    strncpy(park_status.currentplate, plate, plate_length);
    park_status.currentplate[plate_length] = '\0';

    const std::string &light = root["parklightstate"].asString();
    park_status.currentlightstate = get_light_from_str(light);

    auto parkhistory = root["parkhistory"];
#if 0
    for (auto item : parkhistory) {
#else
    //for (unsigned i = 0; i < parkhistory.size(); i++) {
    for (int i = parkhistory.size() - 1; i >= 0; i--) {
        auto item = parkhistory[i];
#endif
        parse_park_history(item);
    }
    return 0;
}

int parkStatus::parse_park_history(const Json::Value &parkhistory)
{
    current_history++;
    if (current_history >= PARK_RECORD_HISTORY_MAX) {
        current_history = 0;
    }

	struct park_history_t *new_node = &(m_history_list_entry[current_history]);

    new_node->index = parkhistory["index"].asInt();
    if (parkhistory["state"].asString() == "驶入") {
        new_node->state = PARK_RECORD_STATE::DRIVE_IN;
    } else {
        new_node->state = PARK_RECORD_STATE::DRIVE_OUT;
    }

    const char* time = parkhistory["time"].asString().c_str();
    const size_t time_length = strlen(time) < TIME_STRING_LENGTH ?
                          strlen(time) : TIME_STRING_LENGTH;
    strncpy(new_node->time, time, time_length);
    (new_node->time)[time_length] = '\0';

    const char* plate = parkhistory["plate"].asString().c_str();
    const size_t plate_length = strlen(plate) < PARK_PLATE_LENGTH ?
                          strlen(plate) : PARK_PLATE_LENGTH;
    strncpy(new_node->plate, plate, plate_length);
    (new_node->plate)[plate_length] = '\0';

    const char* pic_name1 = parkhistory["pic_name1"].asString().c_str();
    const size_t pic1_length = strlen(pic_name1) < PICTURE_NAME_LENGTH ?
                          strlen(pic_name1) : PICTURE_NAME_LENGTH;
    strncpy(new_node->pic_name[0], pic_name1, pic1_length);
    new_node->pic_name[0][pic1_length] = '\0';

    const char* pic_name2 = parkhistory["pic_name2"].asString().c_str();
    const size_t pic2_length = strlen(pic_name2) < PICTURE_NAME_LENGTH ?
                          strlen(pic_name2) : PICTURE_NAME_LENGTH;
    strncpy(new_node->pic_name[1], pic_name2, pic2_length);
    new_node->pic_name[1][pic2_length] = '\0';

    //DEBUG("index = %d", new_node->index);
    //DEBUG("plate = %s", new_node->plate);
    //DEBUG("time = %s", new_node->time);
    return 0;
}
