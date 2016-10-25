package com.areese.example;

import java.nio.ByteBuffer;

import com.yahoo.wildwest.MissingFingers;

public class TestUnsafe implements Runnable {
    static int count = 0;
    static int size = 0;

    public static void main(String[] args) {

        if (args.length >= 1) {
            count = Integer.parseInt(args[0]);
        }

        if (args.length >= 2) {
            size = Integer.parseInt(args[1]);
        }

        TestUnsafe tu = new TestUnsafe();

        tu.run();
    }

    @Override
    public void run() {
        int total = 0;

        for (int i = 0; i < count; i++) {
            // yes it leaks
            //new MissingFingers(size);
            ByteBuffer a = ByteBuffer.allocateDirect(size);
            a.array();
            total += size;
            System.err.println("Allocation total: " + total);
        }
    }
}
