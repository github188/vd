#ifndef __TPBITCOM_COMMON_H__
#define __TPBITCOM_COMMON_H__

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Name:        tpbitcom_retransmition_enable
 * Discription: return the bool value of retransmition
 */
bool tpbitcom_retransmition_enable(void);

/*
 * Name:        tpbitcom_rotate_count
 * Discription: return the rotate_count
 */
int tpbitcom_rotate_count(void);

#ifdef __cplusplus
}
#endif
#endif
