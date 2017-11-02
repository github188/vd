#ifndef __PARK_STATUS_H__
#define __PARK_STATUS_H__

/**
 * @file park_status.h
 * @brief the header file for park status dump to web
 * @author Felix Du <durd07@gmail.com>
 * @version 0.0.1
 * @date 2017-04-27
 */

#include "../../lib/Json/include/json/json.h"

#define TIME_STRING_LENGTH (32)
#define PARK_PLATE_LENGTH (32)
#define SPACE_CODE_LENGTH (32)
#define PARK_RECORD_HISTORY_MAX (10)
#define PICTURE_NAME_LENGTH (128)
#define PARK_PICTURE_NUM (2)

#define PARK_HISTORY_FILE ("/var/www/parkjson/parkhis.json")

enum class PARK_RECORD_STATE
{
    DRIVE_IN = 0,
    DRIVE_OUT = 1
};

enum class CURRENT_PARK_STATE
{
    NOCAR = 0,
    HASCAR = 1
};

enum class CURRENT_LIGHT_STATE
{
    NOLIGHT = 0,
    GREEN,
    GREEN_FLASH,
    RED,
    RED_FLASH,
    BLUE,
    BLUE_FLASH,
    LIGHT_MAX
};

struct park_history_t
{
    unsigned char index;
    char plate[PARK_PLATE_LENGTH];
    PARK_RECORD_STATE state;
    char time[TIME_STRING_LENGTH];
    char pic_name[PARK_PICTURE_NUM][PICTURE_NAME_LENGTH];
};

struct park_status_t
{
    CURRENT_PARK_STATE parkstate;
    char spacecode[SPACE_CODE_LENGTH];
    char currentplate[PARK_PLATE_LENGTH];
    CURRENT_LIGHT_STATE currentlightstate;
    struct park_history_t *history_list;
};

class parkStatus
{
public:
    static int push_record_2_history_list(int, const char*, const char*, const char*, const char*, const char*);

private:
    parkStatus();
    ~parkStatus();
    int dump_to_file(const char* file_name);
    int load_from_file(const char* file_name);

    struct park_status_t m_park_status;
    int parse_from_json(Json::Value &root);
    int parse_park_history(const Json::Value &parkhistory);

    bool get_sd_mount_status();

private:
    struct park_history_t m_history_list_entry[PARK_RECORD_HISTORY_MAX];
    int current_history;
};

extern "C" {
    int push_record_2_history_list(const int objectState,
                                               const char* plate,
                                               const char* park_time,
                                               const char* pic_name1,
                                               const char* pic_name2,
                                               const char* light);
}
#endif /* __PARK_STATUS_H__ */
