g++ -g -O0 -fPIC -I$JAVA_HOME/include/ -I$JAVA_HOME/include/darwin jvmti.cc -dynamiclib -o jvmti.so -lc 
if [ $? -ne 0 ]; then
	exit $?
fi

java -agentpath:./jvmti.so -XX:CompileCommand=exclude,sun/misc/Unsafe::\* -XX:+UnlockDiagnosticVMOptions  -XX:NativeMemoryTracking=summary -cp bin/ com.areese.example.TestUnsafe  $1 $2