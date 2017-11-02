#ifndef _PARKING_LOCK_CONTROL_
#define _PARKING_LOCK_CONTROL_

#ifdef __cplusplus
extern "C" {
#endif

/* common TYPE */
typedef struct _park_lock_operation_t
{
    int (*lock)(void);
    int (*unlock)(void);
    int (*get_status)(void);
    int (*ctrl)(void* args);
}park_lock_operation_t;

extern park_lock_operation_t *g_park_lock_operation;

/* common API */
/**
 * @brief park lock init
 *
 * @param type lock type
 *
 * @return
 */
void park_lock_init(void);

/*
 * Name:        park_lock_lock
 * Description: up the park_lock
 * Paramaters:
 * Return:      0 OK
 *              -1 fail
 */
inline int park_lock_lock(void)
{
    if (g_park_lock_operation != 0)
        return g_park_lock_operation->lock();
    else
        return -1;
}

/*
 * Name:        park_lock_unlock
 * Description: down the park_lock
 * Paramaters:
 * Return:      0 OK
 *              -1 fail
 */
inline int park_lock_unlock(void)
{
    if (g_park_lock_operation != 0)
        return g_park_lock_operation->unlock();
    else
        return -1;
}

/*
 * Name:        park_lock_get_status
 * Description: get the park_lock status
 * Paramaters:
 * Return:      0 OK
 *              -1 fail
 */
inline int park_lock_get_status(void)
{
    if (g_park_lock_operation != 0)
        return g_park_lock_operation->get_status();
    else
        return -1;
}

/*
 * Name:        park_lock_ctrl
 * Description: control the park lock like beep or other actions
 * Paramaters:
 * Return:      0 OK
 *              -1 fail
 */
inline int park_lock_ctrl(void* args)
{
    if (g_park_lock_operation != 0)
        return g_park_lock_operation->ctrl(args);
    else
        return -1;
}

typedef struct _cram_attribute_t
{
    char gw_ip[16];
    unsigned short gw_port;
    char mac[17];
}cram_attribute_t;

typedef struct _park_lock_configuration_t
{
    bool enable;
    int type;
    union {
        cram_attribute_t cram_attribute;
    }attribute;
}park_lock_configuration_t;

extern park_lock_configuration_t park_lock_configuration;

inline int park_lock_get_type(void)
{
    return park_lock_configuration.type;
}

inline bool park_lock_enable(void)
{
    return park_lock_configuration.enable;
}

inline const char* park_lock_cram_gw_ip(void)
{
    return park_lock_configuration.attribute.cram_attribute.gw_ip;
}

inline unsigned short park_lock_cram_port(void)
{
    return park_lock_configuration.attribute.cram_attribute.gw_port;
}

inline const char* park_lock_cram_mac(void)
{
    return park_lock_configuration.attribute.cram_attribute.mac;
}
#ifdef __cplusplus
}
#endif
#endif
