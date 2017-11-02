#ifndef _BAOKANG_PTHREAD_
#define _BAOKANG_PTHREAD_

#ifdef __cplusplus
extern "C" {
#endif

#undef VP_PICTURE_BASESIZE
#define VP_PICTURE_BASESIZE    (1024*400*4)
#define VP_BK_PIC_COUNT       (4)

typedef struct _baokang_info
{
    int  pic_size;
    //char pic[VP_PICTURE_BASESIZE];
    char *pic;
}baokang_info_s;

extern baokang_info_s g_baokang_info[VP_BK_PIC_COUNT];

void* baokang_pthread(void* arg);

#ifdef __cplusplus
}
#endif

#endif
