#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

#include "logger/log.h"
#include "jansson.h"
#include "jwt.h"

/* Macro to allocate a new JWT with checks. */
#define ALLOC_JWT(__jwt) do {		\
	int __ret = jwt_new(__jwt);	\
	ck_assert_int_eq(__ret, 0);	\
	ck_assert_ptr_ne(__jwt, NULL);	\
} while(0)

static unsigned char key[16384] = "keiujr893k45u5m6h3b3by5j3klj5h56";
static size_t key_len = 32;

static void read_key(const char *key_file)
{
	FILE *fp = NULL;
	char key_path[128];
	int ret = 0;

	ret = sprintf(key_path, "/opt/ipnc/.ssh/%s", key_file);

	fp = fopen(key_path, "r");
    if (fp == NULL) {
        return;
    }

	key_len = fread(key, 1, sizeof(key), fp);

	fclose(fp);

	key[key_len] = '\0';
}

/**
 * @brief encode the Authentication JWT header
 *
 * @param space_code
 * @param subject register/drivein/driveout/heartbeat
 *
 * @return the encoded string;
 *         NULL if error occured.
 */
const char* jwt_encode(const char* space_code, const char* subject)
{
    if (NULL == space_code) {
        ERROR("NULL is space_code.");
        return NULL;
    }

    if (NULL == subject) {
        ERROR("NULL is subject.");
        return NULL;
    }

    static bool first = true;
    if (first) {
        read_key("jwt_key");
        first = false;
    }

	jwt_t *jwt = NULL;
	int ret = 0;
	char *out;

    ret = jwt_new(&jwt);
    if ((ret != 0) || (NULL == jwt)) {
        ERROR("jwt_new failed. ret = %d", ret);
        return NULL;
    }

	ret = jwt_add_grant(jwt, "iss", space_code);
	ret = jwt_add_grant(jwt, "sub", subject);


    struct timeval tv;
    gettimeofday(&tv, NULL);

    long iat = tv.tv_sec;
    long exp = tv.tv_sec + 5;

	ret = jwt_add_grant_int(jwt, "iat", iat);
	ret = jwt_add_grant_int(jwt, "exp", exp);

    const jwt_alg_t alg = JWT_ALG_HS256;
    ret = jwt_set_alg(jwt, alg, key, key_len);
	out = jwt_encode_str(jwt);

	jwt_free(jwt);
    return out;
}
