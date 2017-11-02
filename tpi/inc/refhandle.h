/**
 * refhandle.h
 * @brief
 *    RefHandle API
 * @author
 *    ZhangLiang
 * @since
 *    2013-12-26
 * @date
 *    2013-12-26
 */

#ifndef REF_HANDLE_H_INCLUDED
#define REF_HANDLE_H_INCLUDED

#ifdef    __cplusplus
extern "C" {
#endif

#ifdef _MSC_VER
   #pragma warning (disable : 4996)
#endif


#if defined _MSC_VER || WIN32
    #ifndef OS_PLATFORM_WIN
        #define OS_PLATFORM_WIN
    #endif
#endif

#ifdef OS_PLATFORM_WIN
    #include <windows.h>
    #include <process.h>
#else
    #include <pthread.h>
    #include <unistd.h>
#endif

#include "logger/log.h"

/**
 * ref count type
 */
#ifdef OS_PLATFORM_WIN
    typedef volatile unsigned long ref_count_t;
    #define __interlock_inc(add)  InterlockedIncrement(add)
    #define __interlock_dec(sub)  InterlockedDecrement(sub)
#else
    typedef volatile size_t ref_count_t;
    #define __interlock_inc(add)  __sync_add_and_fetch(add, 1)
    #define __interlock_dec(sub)  __sync_sub_and_fetch(sub, 1)
#endif


/**
 * thread lock
 */
#ifdef OS_PLATFORM_WIN
    typedef CRITICAL_SECTION thr_mutex_t;
#else
    typedef pthread_mutex_t  thr_mutex_t;
#endif


/**
 * thread lock
 */
static inline int __thrmutex_init (thr_mutex_t* lock)
{
    int ret = 0;

#ifdef OS_PLATFORM_WIN
    InitializeCriticalSection(lock);
#else
    /* Linux */
    pthread_mutexattr_t  attr;
    ret = pthread_mutexattr_init(&attr);

    if (ret == 0) {
        /* PTHREAD_MUTEX_RECURSIVE_NP ? */
        ret = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE);
        if (ret == 0) {
            ret = pthread_mutex_init(lock, &attr);
        }
        pthread_mutexattr_destroy(&attr);
    }
#endif

    return ret;
}


static inline void __thrmutex_uninit (thr_mutex_t* lock)
{
#ifdef OS_PLATFORM_WIN
    DeleteCriticalSection(lock);
#else
    pthread_mutex_lock(lock); /* do we need this? */
    pthread_mutex_destroy(lock);
#endif
}


static inline void __thrmutex_lock (thr_mutex_t* lock)
{
#ifdef OS_PLATFORM_WIN
    EnterCriticalSection(lock);
#else
    pthread_mutex_lock(lock);
#endif
}


static inline int __thrmutex_trylock (thr_mutex_t* lock)
{
#ifdef OS_PLATFORM_WIN
    return TryEnterCriticalSection(lock)? 0 : (-1);
#else
    return pthread_mutex_trylock(lock);
#endif
}


static inline void __thrmutex_unlock (thr_mutex_t* lock)
{
#ifdef OS_PLATFORM_WIN
    LeaveCriticalSection(lock);
#else
    pthread_mutex_unlock(lock);
#endif
}


#define OBJECT_INVALID (-1)

#define EINDEX (-1)

#define REF_HANDLE_CAST(object,p) \
    RefHandleType * p = (RefHandleType *)(((char*)object)-sizeof(*p))


/**
 * Callback when object just before free self.
 * Used by parent to release its children.
 */
typedef void (* ReleaseFreeDataCallback) (void *handle, void *param);


typedef struct _RefHandleType
{
    ref_count_t __refcount;

    /* DO NOT CHANGE THIS AFTER CREATION */
    int  __htype;

    thr_mutex_t __lock;

    volatile size_t __id;

    void *__handle[0];
} RefHandleType, *RefHandle;


static inline void* _RefHandleCreate (int htype, size_t cbObjectSize)
{
    char *hdl;

    RefHandleType *p = (RefHandleType*) malloc(sizeof(*p) + cbObjectSize);
    if (NULL == p)
        return NULL;

    hdl = (char*) p;
    hdl += sizeof(*p);

    p->__refcount = 1L;
    p->__htype = htype;
    p->__id = EINDEX;
    __thrmutex_init(&p->__lock);

    //DEBUG("alloc the pointer %p %p", p, hdl);
    return (void*) hdl;
}


static inline long _RefHandleRetain (void *object)
{
    if (object) {
        REF_HANDLE_CAST(object, p);
        //DEBUG("increase reference the pointer %p %p", p, object);
        return __interlock_inc(&p->__refcount);
    } else {
        return 0;
    }
}


static inline void _RefHandleRelease (void **ppObject, ReleaseFreeDataCallback freeDataFunc, void *param)
{
    void *object = *ppObject;

    if (object) {
        REF_HANDLE_CAST(object, p);

        if (0 == __interlock_dec(&p->__refcount)) {
            if (freeDataFunc) {
                freeDataFunc(object, param);
            }

            __thrmutex_uninit(&p->__lock);

            free((void*)p);

            *ppObject = 0;
            //DEBUG("actually free the pointer %p %p", p, *ppObject);
        } else {
            //DEBUG("decrease reference the pointer %p %p", p, *ppObject);
        }
    }
}


static inline int _RefHandleGetType (void *object)
{
    if (object) {
        REF_HANDLE_CAST(object, p);
        return p->__htype;
    } else {
        return OBJECT_INVALID;
    }
}


static inline thr_mutex_t * _RefHandleGetLock (void *object)
{
    if (object) {
        REF_HANDLE_CAST(object, p);
        return &p->__lock;
    } else {
        return (thr_mutex_t*) 0;
    }
}


static inline size_t _RefHandleGetId (void *object)
{
    if (object) {
        REF_HANDLE_CAST(object, p);
        return p->__id;
    } else {
        return EINDEX;
    }
}


static inline size_t _RefHandleSetId(void *object, size_t newId)
{
    if (object) {
        size_t oldId;
        REF_HANDLE_CAST(object, p);
        oldId = p->__id;
        p->__id = newId;
        return oldId;
    } else {
        return EINDEX;
    }
}

#ifdef    __cplusplus
}
#endif

#endif /* REF_HANDLE_H_INCLUDED */
