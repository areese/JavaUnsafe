This code exists to make using unsafe a little easier.
I had a use case of passing Inet4Address and Inet6Address to jni and back.
I do it enough that it ends up being an expensive bottleneck.