// Copyright 2016 Yahoo Inc.
// Licensed under the terms of the New-BSD license. Please see LICENSE file in the project root for terms.
#ifndef _Included_addr_support_h
#define _Included_addr_support_h

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>

#define LINUX_AF_INET6 10

class ScopedAddrInfo {
public:
    ScopedAddrInfo(addrinfo *memory) :
            _memory(memory) {
    }

    ScopedAddrInfo() :
            _memory(0) {
    }

    inline void set(addrinfo *memory) {
        _memory = memory;
    }

    inline addrinfo *get() {
        return _memory;
    }

    virtual ~ScopedAddrInfo() {
        if (_memory) {
#ifdef DUMP_DEBUG
            fprintf(stderr, "freeing %p and ai_addr %p\n", _memory,
                    _memory->ai_addr);
            fflush (stderr);
#endif //DUMP_DEBUG
            freeaddrinfo(_memory);
            _memory = 0;
        }
    }
#ifdef __APPLE__
    static const size_t addrInfoSize = sizeof(struct addrinfo);
    static const size_t aiAddrSize = sizeof(struct sockaddr_storage);
#else // __APPLE__
    static const size_t addrInfoSize = sizeof(struct addrinfo)
            + sizeof(struct sockaddr_storage);
#endif // __APPLE__

    static addrinfo *allocAddrInfo(int family) {
        struct addrinfo* ret = (struct addrinfo*) calloc(1, addrInfoSize);

#ifdef __APPLE__
        // os x free's ai_addr.
        ret->ai_addr = (struct sockaddr*) calloc(1,aiAddrSize);
#else
        // addrInfoSize adds the sockaddr on because it seems like glibc doesn't free that:
        // http://osxr.org:8080/glibc/source/sysdeps/posix/getaddrinfo.c#2665
        // if you think you're hitting this, look for malloc to complain, then enable DUMP_DEBUG and compare addresses.
        ret->ai_addr = (struct sockaddr*) (ret + 1);
#endif // __APPLE__

#ifdef DUMP_DEBUG
        fprintf(stderr, "allocated %p and ai_addr %p\n", ret, ret->ai_addr);
#endif //DUMP_DEBUG

        ret->ai_addrlen = sizeof(struct sockaddr_storage);
        ret->ai_family = family;

        return ret;
    }

    static void dumpAddrInfo(const char *str, const struct addrinfo *ptr) {
        (void) str;
        (void) ptr;
#ifdef DUMP_DEBUG
        if (NULL == ptr) {
            fprintf(stderr,"null ptr to dump: %s\n",str);
            return;
        }

        char buf[8192]= {0,};
        ::dumpAddrInfo(str,buf,sizeof(buf),ptr);

        fprintf(stderr,"%s\n",buf);
#endif //DUMP_DEBUG

    }
private:
    // Disabled
    ScopedAddrInfo(const ScopedAddrInfo &);
    void operator=(const ScopedAddrInfo &);

    addrinfo *_memory;
};

#ifndef HAS_IPV6_SOCKADDR
/* multi family IPv4 and IPv6 sockaddr structure */
typedef union {
    struct sockaddr_storage ss;
    struct sockaddr sa;
    struct sockaddr_in sin;
    struct sockaddr_in6 sin6;
} ipv6_sockaddr;
#endif //HAS_IPV6_SOCKADDR

static const size_t in6len = sizeof(in6_addr);
static const size_t in4len = sizeof(in_addr);
static const size_t ipv6_sockaddr_len = sizeof(ipv6_sockaddr);
static const size_t addrInfoSize = sizeof(struct addrinfo)
        + sizeof(ipv6_sockaddr);
/**
 * Take the bytes from Unsafe, and turn them into a sockaddr.
 * Ipv4==address
 * Ipv6==pointer + len
 * madness.
 */
inline void unsafeToSockAddr(ipv6_sockaddr &v6sa, jlong unsafeMemory,
        jint length) {
    memset(&v6sa, 0, sizeof(v6sa));

    if (0 == length) {
        // it's ipv4
        v6sa.sa.sa_family = AF_INET;
        //v6sa.sin.sin_port = htons(port);
        v6sa.sin.sin_addr.s_addr = ntohl(unsafeMemory);
    } else if (4 == length) {
        // it's ipv4
        v6sa.sa.sa_family = AF_INET;

        const char *src = (const char *) unsafeMemory;

        v6sa.sin.sin_addr.s_addr = 0;
        v6sa.sin.sin_addr.s_addr |= ((int) src[0] & 0x00ff) << 0x18;
        v6sa.sin.sin_addr.s_addr |= ((int) src[1] & 0x00ff) << 0x10;
        v6sa.sin.sin_addr.s_addr |= ((int) src[2] & 0x00ff) << 0x08;
        v6sa.sin.sin_addr.s_addr |= ((int) src[3] & 0x00ff);
    } else if (16 == length) {
        // must be ipv6
        v6sa.sa.sa_family = AF_INET6;
        //v6sa.sin6.sin6_port = htons(port);
        memcpy(&v6sa.sin6.sin6_addr.s6_addr, (const void *) unsafeMemory,
                in6len);
    }
}

/**
 * Take the bytes in unsafeAddress, and decode it into an addrInfo structure inside addrInfo
 */
inline int allocAddrInfo(ScopedAddrInfo &addrInfo, long unsafeAddress,
        long length) {
    /* we need at least 6 bytes (1,2,0xc0a8016f) for the shortest thing.
     // 1 address
     // AF_INET (2)
     // 4 byte ip 0xc0a8016f
     * */
    if (0 == unsafeAddress || length < 6) {
        return 0;
    }

    struct addrinfo* head = NULL;

    long currentOffest = 0;
    uint8_t *pointer = (uint8_t *) unsafeAddress;
    // first byte is count.
    uint8_t addressCount = *pointer;

    if (addressCount <= 0) {
        return 0;
    }

    // we use current offset to pass to unsafeToSockAddr
    pointer++;
    currentOffest++;

    for (int i = 0; i < addressCount && currentOffest < length; i++) {
        uint8_t type = *pointer;
        pointer++;
        currentOffest++;

        if ((AF_INET != type && AF_INET6 != type && LINUX_AF_INET6  != type)
                || (AF_INET == type && currentOffest + 4 > length)
                || (AF_INET6 == type && currentOffest + 16 > length)
                || (LINUX_AF_INET6 == type && currentOffest + 16 > length)
                ) {
            break;
        }

        // allocate it in a friendly manner.
        struct addrinfo *current = ScopedAddrInfo::allocAddrInfo(type);

        ipv6_sockaddr *storage = (ipv6_sockaddr *) (current->ai_addr);
        if (AF_INET == type) {
            // we need the next 4 bytes.
            unsafeToSockAddr(*storage, unsafeAddress + currentOffest, 4);
            currentOffest += 4;
            pointer += 4;
        } else if (AF_INET6 == type || LINUX_AF_INET6 == type) {
            // we need the next 16 bytes.
            unsafeToSockAddr(*storage, unsafeAddress + currentOffest, 16);
            currentOffest += 16;
            pointer += 16;
        }

        if (NULL == head) {
            head = current;
            addrInfo.set(head);
        } else {
            head->ai_next = current;
        }
    }

    return 1;
}

/**
 * Take the bytes in an addrinfo structure, and encode it into an unsafe address
 * The unsafe format is:
 * byte totalAddresses
 * byte type  (AF_INET/AF_INET6) (2/10)
 * IPV4:
 *  4 bytes / 32 bits of address.
 * IPV6:
 * 16 bytes / 128 bits of address
 * It returns the total length used.
 */
inline jlong encodeAddrInfoToUnsafe(const addrinfo *addrInfo,
        long unsafeAddress, long length) {
    if (NULL == addrInfo || 0 == unsafeAddress || 0 == length) {
//        fprintf(stderr, "null addrInfo=%p address=0x%lx len=%ld\n", addrInfo,
//                unsafeAddress, length);
        return -1;
    }

    // first we need to iterate all of them and count the size.
    jlong totalLength = 1;
    jlong totalAddresses = 0;

    const addrinfo *current = addrInfo;
    while (NULL != current) {
        if (AF_INET == current->ai_family) {
            totalLength += 5;
            totalAddresses++;
        } else if (AF_INET6 == current->ai_family) {
            totalLength += 17;
            totalAddresses++;
        }
        current = current->ai_next;
    }

    if (totalLength == 1) {
        // no work done.
        return 0;
    }

    if (totalLength > length) {
        // doesn't fit
        //fprintf(stderr, "totalLength %d > length %d\n", totalLength, length);
        return -2;
    }

    if (totalLength > 255) {
        // doesn't fit
        return -3;
    }

    // now we need to walk and encode.
    uint8_t *pointer = (uint8_t*) unsafeAddress;

    size_t index = 0;

    // set length
    pointer[index++] = (uint8_t) totalLength;

    // set addresses
    pointer[index++] = (uint8_t) totalAddresses;

    current = addrInfo;
    while (NULL != current) {
        ipv6_sockaddr *storage = (ipv6_sockaddr *) (current->ai_addr);
        if (AF_INET == current->ai_family) {
            pointer[index++] = AF_INET;
            memcpy(&(pointer[index]), &(storage->sin.sin_addr.s_addr), in4len);
            index += 4;
        } else if (AF_INET6 == current->ai_family) {
            // the java assumes 10, which is linux.
            pointer[index++] = LINUX_AF_INET6;
            memcpy(&(pointer[index]), &(storage->sin6.sin6_addr.s6_addr),
                    in6len);
            // we need the current 16 bytes.
            index += 16;
        }
        current = current->ai_next;
    }

    return index;
}

#endif //_Included_addr_support_h
