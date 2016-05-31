// Copyright 2016 Yahoo Inc.
// Licensed under the terms of the New-BSD license. Please see LICENSE file in the project root for terms.
package com.yahoo.wildwest;

import java.lang.invoke.MethodHandle;
import java.lang.invoke.MethodHandles;
import java.lang.reflect.Constructor;
import java.lang.reflect.Field;
import java.net.Inet4Address;
import java.net.Inet6Address;
import java.net.InetAddress;
import java.net.UnknownHostException;
import java.nio.channels.UnsupportedAddressTypeException;
import java.util.Arrays;
import java.util.Objects;

@SuppressWarnings("restriction")
public class InetAddressAccessor {
    // public static void main(String[] args) throws Exception {
    // testInet4AddressHolder();
    // testInet6AddressHolder();
    // }

    @SuppressWarnings({"rawtypes", "unchecked"})
    static class Inet4AddressHelper {
        Class inet4AddressClass;
        Constructor<InetAddress> inet4AddressConstructor;
        Class inet4AddressHolderClass;
        Field inet4AddressHolderField;
        Field inet4AddressHolderAddressField;
        MethodHandle inet4GetHolderMH;
        MethodHandle inet4HolderGetAddressMH;

        public Inet4AddressHelper() throws ReflectiveOperationException {
            inet4AddressClass = InetAddress.class;
            inet4AddressConstructor = inet4AddressClass.getDeclaredConstructor((Class[]) null);
            inet4AddressConstructor.setAccessible(true);

            for (Class c : inet4AddressClass.getDeclaredClasses()) {
                if ("java.net.InetAddress$InetAddressHolder".equals(c.getName())) {
                    inet4AddressHolderClass = c;
                }
            }

            inet4AddressHolderField = inet4AddressClass.getDeclaredField("holder");
            inet4AddressHolderField.setAccessible(true);

            InetAddress newInstance = inet4AddressConstructor.newInstance();

            Object inet4AddressHolderObject = inet4AddressHolderField.get(newInstance);

            inet4AddressHolderAddressField = inet4AddressHolderObject.getClass().getDeclaredField("address");
            inet4AddressHolderAddressField.setAccessible(true);

            MethodHandles.Lookup lookup = MethodHandles.lookup();
            inet4GetHolderMH = lookup.unreflectGetter(inet4AddressHolderField);
            inet4HolderGetAddressMH = lookup.unreflectGetter(inet4AddressHolderAddressField);
        }

        /**
         * Calls java.net.InetAddress.InetAddressHolder.getAddress() against address to return the 32bit representation.
         * This version uses reflection to get access.
         * 
         * @param address {@link Inet4Address} to get the representation of.
         * @return int representing the address
         */
        int getAddress(Inet4Address address) {
            try {
                Object inet4AddressHolderObject = inet4AddressHolderField.get(address);
                return inet4AddressHolderAddressField.getInt(inet4AddressHolderObject);
            } catch (ReflectiveOperationException e) {
            }
            return 0;
        }

        /**
         * Calls java.net.InetAddress.InetAddressHolder.getAddress() against address to return the 32bit representation.
         * This version uses MethodHandles to get access.
         * 
         * @param address {@link Inet4Address} to get the representation of.
         * @return int representing the address
         */
        int getAddressViaMH(Inet4Address address) {
            try {
                // get the InetAddressHolder object from within address.
                Object holder = inet4GetHolderMH.invoke(address);
                // call int java.net.InetAddress.InetAddressHolder.getAddress()
                return ((Integer) inet4HolderGetAddressMH.invoke(holder)).intValue();
            } catch (Throwable e) {
            }
            return 0;
        }
    }

    @SuppressWarnings({"rawtypes", "unchecked"})
    static class Inet6AddressHelper {
        Class inet6AddressClass;
        Constructor<InetAddress> inet6AddressConstructor;
        Class inet6AddressHolderClass;
        Field inet6AddressHolderField;
        Field inet6AddressHolderAddressField;
        MethodHandle inet6GetHolderMH;
        MethodHandle inet6HolderGetAddressMH;

        public Inet6AddressHelper() throws ReflectiveOperationException {
            inet6AddressClass = Inet6Address.class;
            inet6AddressConstructor = inet6AddressClass.getDeclaredConstructor((Class[]) null);
            inet6AddressConstructor.setAccessible(true);

            for (Class c : inet6AddressClass.getDeclaredClasses()) {
                if ("java.net.InetAddress$Inet6AddressHolder".equals(c.getName())) {
                    inet6AddressHolderClass = c;
                }
            }

            inet6AddressHolderField = inet6AddressClass.getDeclaredField("holder6");
            inet6AddressHolderField.setAccessible(true);

            InetAddress newInstance = inet6AddressConstructor.newInstance();

            Object inet6AddressHolderObject = inet6AddressHolderField.get(newInstance);

            inet6AddressHolderAddressField = inet6AddressHolderObject.getClass().getDeclaredField("ipaddress");
            inet6AddressHolderAddressField.setAccessible(true);

            MethodHandles.Lookup lookup = MethodHandles.lookup();
            inet6GetHolderMH = lookup.unreflectGetter(inet6AddressHolderField);
            inet6HolderGetAddressMH = lookup.unreflectGetter(inet6AddressHolderAddressField);
        }

        byte[] getAddress(Inet6Address address) {
            try {
                Object inet6AddressHolderObject = inet6AddressHolderField.get(address);
                Object value = inet6AddressHolderAddressField.get(inet6AddressHolderObject);
                if (value instanceof byte[]) {
                    return ((byte[]) value);
                }
            } catch (ReflectiveOperationException e) {
            }
            return null;
        }

        byte[] getAddressViaMH(Inet6Address address) {
            try {
                Object holder = inet6GetHolderMH.invoke(address);
                return ((byte[]) inet6HolderGetAddressMH.invoke(holder));
            } catch (Throwable e) {
            }
            return null;
        }
    }

    static final Inet4AddressHelper inet4Helper;
    static final Inet6AddressHelper inet6Helper;

    static {
        try {
            inet4Helper = new Inet4AddressHelper();
            inet6Helper = new Inet6AddressHelper();
        } catch (ReflectiveOperationException e) {
            throw new Error(e);
        }
    }

    public static void load() {

    }

    public static byte[] getAddressViaReflection(Inet6Address address) {
        return inet6Helper.getAddress(address);
    }

    public static long getAddressViaReflection(Inet4Address address) {
        return inet4Helper.getAddress(address);
    }

    public static int getAddressViaMH(Inet4Address address) {
        return inet4Helper.getAddressViaMH(address);
    }

    public static byte[] getAddressViaMH(Inet6Address address) {
        return inet6Helper.getAddressViaMH(address);
    }

    /**
     * Given an InetAddress, get the bytes and store them in an unsafe allocated address If it's ipv4 only use 1 long:
     * addressbytes. If it's ipv6, then it uses the pointer. 192.168.1.111 is returned as the 2 longs: 0xc0a8016f 0x0.
     * While ipv6 would be: 0xsomePointer, 0x10 (128 bit ipv6 address).
     * 
     * In C, this will map nicely to a sockaddr_storage. You might need some custom code to deal with the copy, as you
     * still need to set family.
     * 
     * Also, this completely ignores scope in ipv6. It probably should actually give you a memcpy(sockaddr_storage)
     * friendly version but it doesn't.
     * 
     * @param address input address
     * @return address/length tuple. For ipv4 the tuple is: address/0 so 192.168.1.111 is returned as the 2 longs:
     *         0xc0a8016f 0x0.
     * @throws UnsupportedAddressTypeException if given a non {@link Inet4Address} or non {@link Inet6Address}
     */
    public static MissingFingers powersaw(InetAddress address) {
        Objects.requireNonNull(address);

        long addressLong = 0;
        long len = 0;

        if (address instanceof Inet4Address) {
            addressLong = InetAddressAccessor.getAddressViaMH((Inet4Address) address);
        } else if (address instanceof Inet6Address) {
            byte[] addressBytes = InetAddressAccessor.getAddressViaMH((Inet6Address) address);
            len = addressBytes.length;

            // and now we schlep the bytes via Unsafe.
            addressLong = MUnsafe.getUnsafe().allocateMemory(len);
            MUnsafe.copyMemory(addressLong, len, addressBytes);
        } else {
            throw new UnsupportedAddressTypeException();
        }

        return new MissingFingers(addressLong, len);
    }

    /**
     * <pre>
     * /usr/include/bits/socket.h:#define   PF_INET     2   / * IP protocol family.* /  
     * /usr/include/bits/socket.h:#define  PF_INET6    10  / * IP version 6.  * /
     * /usr/include/bits/socket.h:#define  AF_INET     PF_INET
     * /usr/include/bits/socket.h:#define  AF_INET6    PF_INET6
     * </pre>
     */
    public static final byte AF_INET = 2;
    public static final byte LINUX_AF_INET6 = 10;
    public static final byte OSX_AF_INET6 = 30;
    public static final byte AF_INET6 = LINUX_AF_INET6;

    /**
     * Given an InetAddress[], get the bytes and store them in an unsafe allocated address. Format is: AF_FAMILY, bytes.
     * 
     * 
     * So: If you pass in Inet4Address,Inet6Address, it will take up 1+5 + 17 bytes, or 22 total. 1 byte for family,
     * 4/16 bytes for address. count,2,4bytes,10,16 bytes.
     * 
     * [count],{[type],[address 4/16 bytes]}, {[type],[address 6/16 bytes]},
     * 
     * In C, this will map nicely to a addrinfo. You might need some custom code to deal with the copy, as you still
     * need to set family.
     * 
     * Also, this completely ignores scope in ipv6. It probably should actually give you a memcpy(sockaddr_storage)
     * friendly version but it doesn't.
     * 
     * @param addresses input address
     * @return address/length tuple. For ipv4 the tuple is: address/0 so 192.168.1.111 is returned as the 2 longs:
     *         0xc0a8016f 0x0.
     * @throws UnsupportedAddressTypeException if given a non {@link Inet4Address} or non {@link Inet6Address}
     */
    public static MissingFingers powersaw(InetAddress[] addresses) {
        Objects.requireNonNull(addresses);

        int totalAddresses = addresses.length;
        int ipv4count = 0;
        int ipv6count = 0;

        // address family types.
        byte[] types = new byte[totalAddresses];
        // ipv4 addresses
        int[] ipv4 = new int[totalAddresses];
        // ipv6 addresses
        byte[][] ipv6 = new byte[totalAddresses][];

        for (int i = 0; i < totalAddresses; i++) {
            InetAddress address = addresses[i];
            if (address instanceof Inet4Address) {
                types[i] = AF_INET;
                ipv4[i] = InetAddressAccessor.getAddressViaMH((Inet4Address) address);
                ipv4count++;
            } else if (address instanceof Inet6Address) {
                types[i] = AF_INET6;
                ipv6[i] = InetAddressAccessor.getAddressViaMH((Inet6Address) address);
                ipv6count++;
            } else {
                throw new UnsupportedAddressTypeException();
            }
        }

        // at this point, we should know how many bytes to allocate:
        // 1 * length for the address families.
        // ipv4 count * 4 bytes (heh, we're using 4 bytes instead of the hacky longs I abused everywhere else. 32bit
        // ipv4).
        // ipv6 count * 16 bytes (128 bit ipv6)
        long len = 1 + totalAddresses + (4 * ipv4count) + (16 * ipv6count);

        long addressLong = 0;
        // and now we schlep the bytes via Unsafe.
        addressLong = MUnsafe.getUnsafe().allocateMemory(len);

        long currentAddress = addressLong;

        MUnsafe.putByte(currentAddress, (byte) totalAddresses);
        currentAddress++;

        // we have to iterate and schlep this time.
        for (int i = 0; i < totalAddresses; i++) {
            byte type = types[i];

            MUnsafe.putByte(currentAddress, type);
            currentAddress++;

            if (AF_INET == type) {
                // we're going to write type,int.
                MUnsafe.putInt(currentAddress, ipv4[i]);
                currentAddress += 4;
            } else if (AF_INET6 == type) {
                // we're going to write type, byte[].
                MUnsafe.copyMemory(currentAddress, ipv6[i]);
                currentAddress += 16;
            }
        }

        return new MissingFingers(addressLong, len);
    }


    public static InetAddress newAddress(MissingFingers output) throws UnknownHostException {
        return newAddress(output.getAddress(), output.getLength());
    }

    public static InetAddress newAddress(long address, long length) throws UnknownHostException {
        if (0 == address) {
            return null;
        }

        if (0 != length && 4 != length && 16 != length) {
            throw new IllegalArgumentException("Length must be either 4 (ipv4) or 16 (ipv6), was " + length);
        }

        byte[] bytes;

        if (0 == length) {
            bytes = new byte[4];
            // hacky ipv4.
            // no length. ;), so it's just the ipv4 address
            bytes[0] = (byte) ((address >> 0x18) & 0x0FF);
            bytes[1] = (byte) ((address >> 0x10) & 0x0FF);
            bytes[2] = (byte) ((address >> 0x08) & 0x0FF);
            bytes[3] = (byte) ((address >> 0x00) & 0x0FF);
        } else {
            bytes = new byte[(int) length];

            // in this case, we just copyMemory out
            MUnsafe.copyMemory(bytes, address, length);
        }

        return InetAddress.getByAddress(bytes);
    }


    public static InetAddress[] newAddresses(MissingFingers output) throws UnknownHostException {
        return newAddresses(output.getAddress(), output.getLength());
    }

    /**
     * Given address, which was encoded to from addrinfo *, decode it. Encoding is: byte length, byte type
     * (AF_INET/AF_INET6), 4 bytes ipv4/16 bytes ipv6
     * 
     * @param address data was written to.
     * @param length length of data written.
     * @return Array if {@link InetAddress}
     * @throws UnknownHostException on failure
     */
    public static InetAddress[] newAddresses(long address, long length) throws UnknownHostException {
        if (0 == address || 0 == length) {
            return null;
        }

        long currentAddress = address;

        byte totalLength = MUnsafe.getByte(currentAddress++);
        byte totalAddresses = MUnsafe.getByte(currentAddress++);

        byte[] allTheBytes = new byte[(int) length];
        MUnsafe.copyMemory(allTheBytes, address, length);

        if (totalLength < 6 || 0 == totalAddresses) {
            return new InetAddress[] {};
        }

        InetAddress[] addresses = new InetAddress[totalAddresses];

        for (int i = 0; i < totalAddresses; i++) {
            byte type = MUnsafe.getByte(currentAddress++);
            byte[] bytes;

            int len = 0;
            if (AF_INET == type) {
                len = 4;
            } else if (AF_INET6 == type) {
                len = 16;
            } else {
                throw new UnknownHostException("At index " + i + " Unknown type " + type + " found in bytes: "
                                + Arrays.toString(allTheBytes));
            }

            bytes = new byte[len];
            // in this case, we just copyMemory out
            MUnsafe.copyMemory(bytes, currentAddress, len);
            addresses[i] = InetAddress.getByAddress(bytes);

            currentAddress += len;
        }

        return addresses;
    }

}
