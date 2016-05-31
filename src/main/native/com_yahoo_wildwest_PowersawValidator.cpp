// Copyright 2016 Yahoo Inc.
// Licensed under the terms of the New-BSD license. Please see LICENSE file in the project root for terms.
#include <jni.h>
#include <arpa/inet.h>
#include "addr_support.h"
#include "com_yahoo_wildwest_PowersawValidator.h"

#define EXPECTED_IPV4 "192.168.1.111"
#define EXPECTED_IPV6 "2001:4998:0:1::1007"

int checkAddress(const char *expected, int ai_family, const ipv6_sockaddr *addr,
        const char *forwho) {
    if (NULL == expected && NULL == addr) {
        return 0;
    }

    if (NULL == expected) {
        return -1;
    }

    if (NULL == addr) {
        return -2;
    }

    const void *addr_s = NULL;
    /* get the IP address as string passed to this function */
    char ipstr[INET6_ADDRSTRLEN] = { 0, };
    if (ai_family == AF_INET) {
        /* IPv4 */
        const struct sockaddr_in *ipv4 = &(addr->sin);
        addr_s = (const void*) &(ipv4->sin_addr);
    } else {
        /* IPv6 */
        const struct sockaddr_in6 *ipv6 = &(addr->sin6);
        addr_s = (const void*) &(ipv6->sin6_addr);
    }

    if (LINUX_AF_INET6 == ai_family) {
        ai_family=AF_INET6;
    }

    if (NULL == inet_ntop(ai_family, addr_s, ipstr, sizeof(ipstr))) {
        return -3;
    }

    fprintf(stderr, "comparing %s to %s for %s\n", expected, ipstr, forwho);
    return strcmp(expected, ipstr);
}

int checkAddress(const char *expected, const struct addrinfo *addr,
        const char *forwho) {
    if (NULL == expected && NULL == addr) {
        return 0;
    }

    if (NULL == expected) {
        return -1;
    }

    if (NULL == addr) {
        return -2;
    }

    return checkAddress(expected, addr->ai_family,
            (const ipv6_sockaddr*) addr->ai_addr, forwho);
}

/*
 * Class:     com_yahoo_wildwest_PowersawValidator
 * Method:    validateIpv4
 * Signature: (JJ)I
 */
JNIEXPORT jint JNICALL Java_com_yahoo_wildwest_PowersawValidator_validateIpv4(
        JNIEnv *, jclass, jlong address, jlong length) {
    fprintf(stderr, "calling validateIpv6\n");

    ipv6_sockaddr v6sa;
    memset(&v6sa, 0, sizeof(ipv6_sockaddr));
    unsafeToSockAddr(v6sa, address, length);
    return checkAddress(EXPECTED_IPV4, AF_INET, &v6sa, "validateIpv4");
}

/*
 * Class:     com_yahoo_wildwest_PowersawValidator
 * Method:    validateIpv6
 * Signature: (JJ)I
 */
JNIEXPORT jint JNICALL Java_com_yahoo_wildwest_PowersawValidator_validateIpv6(
        JNIEnv *, jclass, jlong address, jlong length) {
    fprintf(stderr, "calling validateIpv6\n");

    ipv6_sockaddr v6sa;
    memset(&v6sa, 0, sizeof(ipv6_sockaddr));
    unsafeToSockAddr(v6sa, address, length);
    return checkAddress(EXPECTED_IPV6, AF_INET6, &v6sa, "validateIpv6");
}

/*
 * Class:     com_yahoo_wildwest_PowersawValidator
 * Method:    validateAddresses
 * Signature: (JJ)I
 */
JNIEXPORT jint JNICALL Java_com_yahoo_wildwest_PowersawValidator_validateAddresses(
        JNIEnv *, jclass, jlong addresses, jlong length) {

    fprintf(stderr, "calling validateAddresses\n");

    ScopedAddrInfo aiScope;
    if (!allocAddrInfo(aiScope, addresses, length)) {
        return -2;
    }

    int ret;

    // check the first address
    ret = checkAddress(EXPECTED_IPV4, aiScope.get()->ai_family,
            (const ipv6_sockaddr*) aiScope.get()->ai_addr,
            "validateAddresses:validateIpv4");

    if (0 != ret) {
        return ret;
    }

    // should not be lazy and loop all of them.
    // that would be way better testing.
    if (NULL != aiScope.get()->ai_next) {
        addrinfo *next = aiScope.get()->ai_next;
        ret = checkAddress(EXPECTED_IPV6, next->ai_family,
                (const ipv6_sockaddr*) next->ai_addr,
                "validateAddresses:validateIpv6");

        if (0 != ret) {
            return ret;
        }
    }

    return 0;
}

/*
 * Class:     com_yahoo_wildwest_PowersawValidator
 * Method:    copyAddresses
 * Signature: (JJ)J
 */
JNIEXPORT jlong JNICALL Java_com_yahoo_wildwest_PowersawValidator_copyAddresses(
        JNIEnv *, jclass, jlong address, jlong length) {

    fprintf(stderr, "calling copyAddresses: 0x%lx %ld\n", (unsigned long)address, (unsigned long)length);

    ScopedAddrInfo aiScope;

    addrinfo *v6 = ScopedAddrInfo::allocAddrInfo(AF_INET6);
    if (NULL == v6) {
        return -1;
    }

    addrinfo *v4 = ScopedAddrInfo::allocAddrInfo(AF_INET);
    if (NULL == v4) {
        return -1;
    }

    int ret = 0;
    ret = inet_pton(AF_INET6, EXPECTED_IPV6, v6->ai_addr);
    if (1 != ret) {
        fprintf(stderr, "Failed inet_pton\n");
        return -2;
    }

    ret = inet_pton(AF_INET, EXPECTED_IPV4, v4->ai_addr);
    if (1 != ret) {
        fprintf(stderr, "Failed inet_pton\n");
        return -2;
    }

    // chain v6 -> v4
    v6->ai_next = v4;

    // and scope it.
    aiScope.set(v6);

    // and now we are down to the part we are testing.
    ret = encodeAddrInfoToUnsafe(aiScope.get(), address, length);
    fprintf(stderr, "returning %d\n", ret);

    return ret;
}

