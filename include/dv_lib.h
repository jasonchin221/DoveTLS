#ifndef __DV_LIB_H__
#define __DV_LIB_H__

#include <arpa/inet.h>
#include <string.h>

#define dv_offsetof(TYPE, MEMBER) ((size_t) &((TYPE *)0)->MEMBER)

/*
 * dv_container_of - cast a member of a structure out to the containing structure
 * @ptr:    the pointer to the member.
 * @type:   the type of the container struct this is embedded in.
 * @member: the name of the member within the struct.
 */
#define dv_container_of(ptr, type, member) ({          \
        const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
        (type *)( (char *)__mptr - offsetof(type,member) );})

#define DV_ARRAY_SIZE(array)    (sizeof(array)/sizeof(array[0]))

#define DV_HTONS(a)     htons(a)
#define DV_HTONL(a)     htonl(a)

#define DV_SET_LENGTH(dest, value) \
    do { \
        typeof(value)   n; \
        char            tmp[sizeof(value)]; \
        n = DV_HTONL(value); \
        memcpy(tmp, &n, sizeof(tmp)); \
        memcpy(dest, &tmp[sizeof(tmp) - sizeof(dest)], sizeof(dest)); \
    } while (0)

#endif