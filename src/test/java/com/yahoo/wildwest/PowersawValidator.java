package com.yahoo.wildwest;

class PowersawValidator {
    static {
        JniLibraryLoader.load();
    }

    /**
     * validates that address is 192.168.1.111
     * 
     * @param address address
     * @param length length
     * @return 0 if valid.
     */
    static final native int validateIpv4(long address, long length);

    /**
     * validates that address is [2001:4998:0:1::1007]
     * 
     * @param address address
     * @param length length
     * @return 0 if valid.
     */
    static final native int validateIpv6(long address, long length);

    /**
     * validates that address is 192.168.1.111, [2001:4998:0:1::1007]
     * 
     * @param address address
     * @param length length
     * @return 0 if valid.
     */
    static final native int validateAddresses(long address, long length);

    /**
     * validates that encodeAddrInfoToUnsafe works. encodes 192.168.1.111, [2001:4998:0:1::1007]
     * 
     * @param address address
     * @param length length
     * @return 0 if valid.
     */
    static final native long copyAddresses(long address, long length);
}
