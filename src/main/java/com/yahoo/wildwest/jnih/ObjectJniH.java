package com.yahoo.wildwest.jnih;

import java.io.IOException;
import java.io.PrintWriter;
import java.io.StringWriter;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Set;

import com.yahoo.example.test.DumpTest;

/**
 * Given an SIMPLE object on the classpath Generate all of the stub code to copy it into a long/long (address, length)
 * combo that can be passed to jni and the static c function to decode it.
 * 
 * @author areese
 *
 */
public class ObjectJniH {

    static final String GET_LONG_VALUE_STRING = "MUnsafe.unsafe.getLong(address + offset);";
    static final String FOUR_SPACE_TAB = "    ";

    private final Set<String> blacklistedMethods = generateBlackList();
    private final Class<?> objectClass;
    private final String objectClassName;
    private final String shortObjectName;

    public ObjectJniH(Class<?> objectClass) {
        this.objectClass = objectClass;
        this.objectClassName = this.objectClass.getName();
        String[] temp = objectClassName.split("\\.");
        this.shortObjectName = temp[temp.length - 1];

    }


    private static Set<String> generateBlackList() {
        Set<String> blacklistedMethods = new HashSet<>();
        for (Method m : Object.class.getMethods()) {
            blacklistedMethods.add(m.getName());
        }

        for (Method m : Class.class.getMethods()) {
            blacklistedMethods.add(m.getName());
        }

        return blacklistedMethods;
    }


    boolean isBlacklisted(String methodName, Class<?> returnType, AccessorType methodType) {
        if (blacklistedMethods.contains(methodName)) {
            System.err.println(methodName + " is from Object");
            return true;
        }

        if (returnType.isPrimitive() && returnType.equals(Void.TYPE)) {
            System.err.println(methodName + " returns void");
            return true;
        }

        if (null != methodType) {
            return methodType.isBlacklisted(methodName);
        }

        return false;
    }

    public List<Method> findGetters() {
        // first we need to find all of it's fields, since we're generating code.
        // I'm only looking for getters. If you don't have getters, it won't be written.
        List<Method> getters = new LinkedList<>();

        for (Method m : objectClass.getMethods()) {
            String methodName = m.getName();

            if (isBlacklisted(methodName, m.getReturnType(), AccessorType.GETTER)) {
                continue;
            }

            System.out.println("added " + methodName);
            getters.add(m);
        }

        return getters;
    }


    public List<Method> findSetters() {
        // first we need to find all of it's fields, since we're generating code.
        // I'm only looking for getters. If you don't have getters, it won't be written.
        List<Method> setters = new LinkedList<>();

        for (Method m : objectClass.getMethods()) {
            String methodName = m.getName();

            if (isBlacklisted(methodName, m.getReturnType(), AccessorType.SETTER)) {
                continue;
            }

            System.out.println("added " + methodName);
            setters.add(m);
        }

        return setters;
    }


    public String createCStruct() {
        // first we need to find all of it's fields, since we're generating code.
        // I'm only looking for getters. If you don't have getters, it won't be written.
        // List<Field> fields = new LinkedList<>();

        StringBuilder structString = new StringBuilder();
        String structName = shortObjectName + "Struct";
        structString.append("typedef struct " + structName + " {\n");

        parseObject(objectClass, (ctype, field, type) -> {
            switch (ctype) {
                case STRING:
                    structString.append(FOUR_SPACE_TAB + "uint64_t " + field.getName() + "Bytes;\n");
                    structString.append(FOUR_SPACE_TAB + "uint64_t " + field.getName() + "Len;\n");
                    break;

                case LONG:
                case INT:
                    // yes we waste 32 bits.

                case BYTE:
                    // yes we waste 56 bits.
                        structString.append(FOUR_SPACE_TAB + "uint64_t " + field.getName() + "; // " + type.getName()
                                        + "\n");
                    break;

                default:
                    structString.append(FOUR_SPACE_TAB + "DATASTRUCT " + field.getName() + "; // " + type.getName()
                                    + "\n");
                    break;

            }

            // System.out.println("field " + ctype + " " + fieldName + " " + f.isAccessible());
            // fields.add(f);
        });

        structString.append("} " + structName + ";\n");

        return structString.toString();

        // return fields;
    }


    public String createJavaCodeBlock() throws IOException {
        try (StringWriter sw = new StringWriter(); PrintWriter pw = new PrintWriter(sw)) {

            pw.println("public " + objectClassName + " create" + shortObjectName + "(long address, long len) {");
            setupJavaVariablesBlock(pw);
            createBitSpitter(pw);
            createConstructorInvocation(pw);
            pw.println("}");
            return sw.toString();
        }
    }

    private void setupJavaVariablesBlock(PrintWriter pw) {
        // first we need to find all of it's fields, since we're generating code.
        // I'm only looking for getters. If you don't have getters, it won't be written.
        // List<Field> fields = new LinkedList<>();

        parseObject(objectClass, (ctype, field, type) -> {
            switch (ctype) {
                case STRING:
                    pw.println(FOUR_SPACE_TAB + "// TOOD: support String");
                    pw.println(FOUR_SPACE_TAB + "long " + field.getName() + "Len;");
                    pw.println(FOUR_SPACE_TAB + "byte[] " + field.getName() + "Bytes;");
                    // = new byte["+ field.getName() + "Len];\n");

                        pw.println(FOUR_SPACE_TAB + "String " + field.getName() + ";");
                    // pw.println(" = new String(" + field.getName()
                    // + "Bytes, StandardCharsets.UTF_8);\n");
                        break;

                case LONG:
                    pw.println(FOUR_SPACE_TAB + "long " + field.getName() + "; // " + type.getName());
                    break;

                case INT:
                    pw.println(FOUR_SPACE_TAB + "int " + field.getName() + "; // " + type.getName());
                    break;

                case BYTE:
                    pw.println(FOUR_SPACE_TAB + "byte " + field.getName() + "; // " + type.getName());
                    break;

                default:
                    pw.println(FOUR_SPACE_TAB + "// TOOD: support " + type.getName());
                    break;
            }
        });

        pw.println();
    }

    private void createConstructorInvocation(PrintWriter pw) {

        StringBuilder constructorString = new StringBuilder();
        // really shouldn't name things so terribly
        constructorString.append(FOUR_SPACE_TAB + objectClassName + " newObject = new " + objectClassName + "(");
        constructorString.append("\n");

        String trailer = ", // \n";

        parseObject(objectClass, (ctype, field, type) -> {
            // how many bytes do we skip? Strings are long,long so 16, everything else is 8 byte longs until we stop
            // wasting bits.
                        constructorString.append(FOUR_SPACE_TAB + FOUR_SPACE_TAB + field.getName()).append(trailer);
                    });

        // remove the extra comma
        int index = constructorString.lastIndexOf(",");
        if (-1 != index) {
            constructorString.delete(index, constructorString.length());
        }
        constructorString.append(");\n");

        pw.println(constructorString.toString());
        pw.println(FOUR_SPACE_TAB + "return newObject;");
    }


    private void createBitSpitter(PrintWriter pw) {
        // assume address, len
        pw.println("long offset = 0;");

        // how many bytes do we skip? Strings are long,long so 16, everything else is 8 byte longs until we stop
        // wasting bits.
        parseObject(objectClass,
                        (ctype, field, type) -> {
                            String fieldName = field.getName();
                            int offsetBy = 0;
                            switch (ctype) {
                                case STRING:
                                    offsetBy = 8;
                                    pw.println(FOUR_SPACE_TAB + fieldName + "Bytes = " + GET_LONG_VALUE_STRING);
                                    pw.println(FOUR_SPACE_TAB + "offset += " + offsetBy + "; // just read " + fieldName
                                                    + " type " + type.getName());
                                    pw.println();

                                    pw.println(FOUR_SPACE_TAB + fieldName + "Len = " + GET_LONG_VALUE_STRING);
                                    pw.println(FOUR_SPACE_TAB + "offset += " + offsetBy + "; // just read " + fieldName
                                                    + " type " + type.getName());
                                    pw.println();
                                    pw.println(FOUR_SPACE_TAB + "if (null != " + fieldName + "Bytes && null != "
                                                    + fieldName + "Len) {");
                                    pw.println(FOUR_SPACE_TAB + FOUR_SPACE_TAB + fieldName + " = new String("
                                                    + fieldName + "Bytes, 0, " + fieldName
                                                    + "Len, StandardCharsets.UTF_8);");
                                    pw.println(FOUR_SPACE_TAB + "} else {");
                                    pw.println(FOUR_SPACE_TAB + FOUR_SPACE_TAB + fieldName + " = null;");
                                    pw.println(FOUR_SPACE_TAB + "}");
                                    pw.println();
                                    break;

                                case LONG:
                                    offsetBy = 8;
                                    pw.println(FOUR_SPACE_TAB + fieldName + " = " + GET_LONG_VALUE_STRING);
                                    pw.println(FOUR_SPACE_TAB + "offset += " + offsetBy + "; // just read " + fieldName
                                                    + " type " + type.getName());
                                    break;

                                case INT:
                                    offsetBy = 8;
                                    pw.println(FOUR_SPACE_TAB + fieldName + " = (int)" + GET_LONG_VALUE_STRING);
                                    pw.println("offset += " + offsetBy + "; // just read " + fieldName + " type "
                                                    + type.getName());
                                    break;


                                case BYTE:
                                    offsetBy = 8;
                                    pw.println(FOUR_SPACE_TAB + fieldName + " = (byte)" + GET_LONG_VALUE_STRING);
                                    pw.println(FOUR_SPACE_TAB + "offset += " + offsetBy + "; // just read " + fieldName
                                                    + " type " + type.getName());
                                    break;

                            }

                            pw.println();

                            // System.out.println("field " + ctype + " " + fieldName + " " + f.isAccessible());
                            // fields.add(f);
                        });

        pw.println();
    }

    /**
     * Helper function to walk the fields of a class and write out either jni or java wrapper bits we'll need.
     * 
     * @param objectClass Class to operate on, expects primitives + strings, no arrays
     * @param pt lambda to invoke on each field that is a primitive or string.
     */
    public static void parseObject(Class<?> objectClass, ProcessType pt) {
        for (Field field : objectClass.getDeclaredFields()) {

            Class<?> type = field.getType();

            if (!type.isPrimitive() && !type.isInstance("") || type.isArray()) {
                continue;
            }

            CTYPES ctype = CTYPES.getCType(type);

            pt.process(ctype, field, type);
        }
    }



    public static void main(String[] args) throws Exception {

        Class<?> classToDump;
        boolean generateCCode = false;
        boolean generateJavaCode = false;

        if (args.length > 0) {
            classToDump = Class.forName(args[0]);
        } else {
            classToDump = new DumpTest().getClass();
        }

        if (args.length > 1) {
            if ("-cstruct".equals(args[1])) {
                generateCCode = true;
            }
            if ("-java".equals(args[1])) {
                generateJavaCode = true;
            }
        }

        ObjectJniH ojh = new ObjectJniH(classToDump);

        if (generateCCode) {
            // create the c struct
            String cstructString = ojh.createCCodeBlock();
            System.out.println(cstructString);
        }

        if (generateJavaCode) {
            String javaString = ojh.createJavaCodeBlock();
            System.out.println(javaString);
        }

        // create the java read code, we can use the setters we've found

        // some people take constructors.
        // we can do that by making either:
        // a) a list of getLongs()
        // b) a list of longs assigned from getLong

    }


    private String createCCodeBlock() {
        // TODO Auto-generated method stub
        return null;
    }
}