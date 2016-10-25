package com.areese.example;

import com.sun.btrace.annotations.*;
import static com.sun.btrace.BTraceUtils.*;

import com.sun.btrace.AnyType;

@BTrace
public class BtraceTracer {
    @OnMethod(clazz = "/java\\.nio\\..*/", method = "/.*/")
    public static void m(@Self Object o, @ProbeClassName String probeClass, @ProbeMethodName String probeMethod) {
        println("this = " + o);
        print("entered " + probeClass);
        println("." + probeMethod);
    }
}
