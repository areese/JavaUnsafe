#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "jvmti.h"
#include "jni.h"

// Function pointers for the native functions we need to replace
typedef jlong (*reallocateMemoryFptr)(JNIEnv *env, jobject unsafe, jlong addr, jlong size);
typedef jlong (*allocateMemoryFptr)(JNIEnv *env, jobject unsafe, jlong size);


// Global gdata.that we're using.
typedef struct GlobalAgentdata {
  jvmtiEnv *jvmti;
  jrawMonitorID lock;
  JavaVM* jvm;
  reallocateMemoryFptr oldReallocateMemory;
  allocateMemoryFptr oldAllocateMemory;
} GlobalAgentdata;

// global gdata.
GlobalAgentdata gdata;


/**
 * This function will release a given pointer using Deallocate,
 * and overwrite the old pointer to now point at NULL
 */
static inline void release(jvmtiEnv *jvmti_env, unsigned char **p) {
  if (NULL == p) {
    return;
  }

  if (NULL == *p) {
    return;
  }

  jvmti_env->Deallocate(*p);

  *p = NULL;
}

/**
 * This class makes keeping track of jvmti allocated memory easier
 */
class ScopedJvmtiPointer {
public:
  ScopedJvmtiPointer(jvmtiEnv *jvmti_env) : jvmti_env(jvmti_env), vp(NULL)
  {}

  ScopedJvmtiPointer(jvmtiEnv *jvmti_env, void *ip) : jvmti_env(jvmti_env), vp(ip)
  {}

  ScopedJvmtiPointer(jvmtiEnv *jvmti_env, char *ip) : jvmti_env(jvmti_env), cp(ip)
  {}

  ~ScopedJvmtiPointer()
  { ::release(jvmti_env, &ucp); }

  void set(void *ip) {
    this->vp = ip;
  }

  void set(char *ip) {
    this->cp = ip;
  }

  void *get() {
    return this->vp;
  }

  char *str() {
    return this->cp;
  }

  void **address() {
    return &this->vp;
  }

  char **strAddress() {
    return &this->cp;
  }

private:
  union {
    void *vp;
    char *cp;
    unsigned char *ucp;
  };

  jvmtiEnv *jvmti_env;
};

/**
 * This function checks for a jvmti error, and prints out the error release
 */
static void check_jvmti_error(jvmtiEnv *jvmti_env, jvmtiError errnum, const char *reason)
{
  if (errnum == JVMTI_ERROR_NONE) {
    return;
  }
   
  ScopedJvmtiPointer errString(jvmti_env);

  jvmti_env->GetErrorName(errnum, errString.strAddress());

  fprintf(stderr, "ERROR: JVMTI: %d(%s): %s\n", errnum,
      (NULL == errString.str() ? "Unknown" : errString.str()),
      (NULL == reason ? "" : reason));
}

/* Enter a critical section by doing a JVMTI Raw Monitor Enter */
static inline void enter_critical_section(jvmtiEnv *jvmti_env) {
  jvmtiError error;

  error = jvmti_env->RawMonitorEnter(gdata.lock);
  check_jvmti_error(jvmti_env, error, "Cannot enter with raw monitor");
}

/* Exit a critical section by doing a JVMTI Raw Monitor Exit */
static inline void exit_critical_section(jvmtiEnv *jvmti_env) {
  jvmtiError error;

  error = jvmti_env->RawMonitorExit(gdata.lock);
  check_jvmti_error(jvmti_env, error, "Cannot exit with raw monitor");
}

class ScopedCriticalSection {
public:
  ScopedCriticalSection(jvmtiEnv *jvmti_env) :jvmti_env(jvmti_env)
  {
    enter_critical_section(jvmti_env);
  }

  ~ScopedCriticalSection() {
    exit_critical_section(jvmti_env);
  }

private:
  jvmtiEnv *jvmti_env;
};

/**
 * Given a jvmti phase, return the human readable string
 */
static const char *getPhaseString(jvmtiPhase p) {
  switch (p) {
    case JVMTI_PHASE_ONLOAD: return "JVMTI_PHASE_ONLOAD";
    case JVMTI_PHASE_PRIMORDIAL: return "JVMTI_PHASE_PRIMORDIAL";
    case JVMTI_PHASE_START: return "JVMTI_PHASE_START";
    case JVMTI_PHASE_LIVE: return "JVMTI_PHASE_LIVE";
    case JVMTI_PHASE_DEAD: return "JVMTI_PHASE_DEAD";
    default: return "huh?";
  }

  return "huh?";
}

/**
 * This function will dump the first 5 frames of the stack if
 * called while the jvm is live
 */
static void dump_stack_in_jvmti(jvmtiEnv *jvmti_env, const char *prefix) {
  jvmtiError err = JVMTI_ERROR_NONE;

  jvmtiPhase phase;
  err = jvmti_env->GetPhase(&phase);
  if (phase != JVMTI_PHASE_LIVE) {
    return;
  }

  fprintf(stderr,"%s dump: phase %s\n", prefix, getPhaseString(phase));

  jvmtiFrameInfo frames[5] = {0,};
  jint count = 0;

  err = jvmti_env->GetStackTrace(NULL, 0, 5, frames, &count);
  check_jvmti_error(jvmti_env, err, "GetStackTrace failed\n");

  if (JVMTI_ERROR_NONE != err || count <= 1) {
    return;
  }

  for (int i = 0; i < count; i++) {
    jclass klass = NULL;
    err = jvmti_env->GetMethodDeclaringClass(frames[i].method, &klass);
    check_jvmti_error(jvmti_env, err, "GetMethodDeclaringClass failed\n");

    if (JVMTI_ERROR_NONE != err) {
      continue;
    }

    ScopedJvmtiPointer framesMethodName(jvmti_env);
    ScopedJvmtiPointer framesSignaturePtr(jvmti_env);
    ScopedJvmtiPointer framesGenericPtr(jvmti_env);

    err = jvmti_env->GetMethodName(frames[i].method,
      framesMethodName.strAddress(),
      framesSignaturePtr.strAddress(),
      framesGenericPtr.strAddress());

    check_jvmti_error(jvmti_env, err, "GetMethodName2 failed\n");
    if (JVMTI_ERROR_NONE != err) {
      continue;
    }

    ScopedJvmtiPointer classSignature(jvmti_env);
    ScopedJvmtiPointer classGeneric(jvmti_env);

    err = jvmti_env->GetClassSignature(klass, classSignature.strAddress(), classGeneric.strAddress());
    check_jvmti_error(jvmti_env, err, "GetClassSignature failed\n");

    // FIXME: can add the parameters size if it's not a native function.
    jint paramsSize = 0;
    // if (err == JVMTI_ERROR_NONE) {
    //   err = jvmti_env->GetArgumentsSize(frames[i].method, &paramsSize);
    //   check_jvmti_error(jvmti_env, err, "GetArgumentsSize failed\n");
    // }
    fprintf(stderr,"%s [%d] %s::%s  (%d)\n", prefix, i, classSignature.str(), framesMethodName.str(), paramsSize);
  }
}

jlong new_Unsafe_ReallocateMemory(JNIEnv *env, jobject unsafe, jlong addr, jlong size)
{
  fprintf(stderr, "reallocating %ld bytes for %lx\n", size, addr);
  dump_stack_in_jvmti(gdata.jvmti, "new_Unsafe_ReallocateMemory");
  return (*gdata.oldReallocateMemory)(env, unsafe, addr, size);
}

jlong new_Unsafe_AllocateMemory(JNIEnv *env, jobject unsafe, jlong size)
{
  fprintf(stderr, "allocating %ld bytes\n", size);
  dump_stack_in_jvmti(gdata.jvmti, "new_Unsafe_AllocateMemory");
  return (*gdata.oldAllocateMemory)(env, unsafe, size);
}

JNICALL void cbMethodEntry
    (jvmtiEnv *jvmti_env,
     JNIEnv* jni_env,
     jthread thread,
     jmethodID method)
{
  ScopedCriticalSection cs(jvmti_env);

  ScopedJvmtiPointer methodName(jvmti_env);
  ScopedJvmtiPointer signature(jvmti_env);
  ScopedJvmtiPointer generic(jvmti_env);

  jvmtiError err = JVMTI_ERROR_NONE;

  err = jvmti_env->GetMethodName(method, methodName.strAddress(), signature.strAddress(), generic.strAddress());
  check_jvmti_error(jvmti_env, err, "GetMethodName failed\n");

  if (JVMTI_ERROR_NONE != err) {
    return;
  }

  bool isAllocate = (0 == strcmp(methodName.str(),"allocateMemory"));
  bool isReAllocate = (0 == strcmp(methodName.str(), "reallocateMemory"));

  if (!isAllocate && !isReAllocate) {
    return;
  }

  fprintf(stderr, "in cbMethodEntry\n");
  dump_stack_in_jvmti(jvmti_env, "cbMethodEntry");
}

JNICALL void cbMethodExit
    (jvmtiEnv *jvmti_env,
     JNIEnv* jni_env,
     jthread thread,
     jmethodID method,
     jboolean was_popped_by_exception,
     jvalue return_value)
{
}

JNICALL void cbNativeMethodBind
    (jvmtiEnv *jvmti_env,
     JNIEnv* jni_env,
     jthread thread,
     jmethodID method,
     void* address,
     void** new_address_ptr)
{
  ScopedCriticalSection cs(jvmti_env);

  // default to nothing.
  *new_address_ptr = address;

  ScopedJvmtiPointer methodName(jvmti_env);
  ScopedJvmtiPointer signature(jvmti_env);
  ScopedJvmtiPointer generic(jvmti_env);
  jvmtiError err = JVMTI_ERROR_NONE;

  err = jvmti_env->GetMethodName(method, methodName.strAddress(), signature.strAddress(), generic.strAddress());
  check_jvmti_error(jvmti_env, err, "GetMethodName failed\n");
  if (JVMTI_ERROR_NONE != err) {
    return;
  }

  bool isAllocate = (0 == strcmp(methodName.str(),"allocateMemory"));
  bool isReAllocate = (0 == strcmp(methodName.str(), "reallocateMemory"));

  if (!isAllocate && !isReAllocate) {
    exit_critical_section(jvmti_env);
    return;
  }
  fprintf(stderr, "in cbNativeMethodBind\n");

  if (isAllocate) {
    // if (NULL == oldAllocateMemory) {
    gdata.oldAllocateMemory = (allocateMemoryFptr)address;
    *new_address_ptr = (void*)new_Unsafe_AllocateMemory;
    // } else {
    //   *new_address_ptr = address;
    //   exit_critical_section(jvmti_env);
    //   return;
    // }
  }

  if (isReAllocate) {
    // if (NULL == oldReallocateMemory) {
    gdata.oldReallocateMemory = (reallocateMemoryFptr)address;
    *new_address_ptr = (void*)new_Unsafe_ReallocateMemory;
    // } else {
    //   *new_address_ptr = address;
    //   exit_critical_section(jvmti_env);
    //   return;
    // }
  }

  jclass klass = NULL;
  ScopedJvmtiPointer classSignature(jvmti_env);
  ScopedJvmtiPointer classGeneric(jvmti_env);

  err = jvmti_env->GetMethodDeclaringClass(method, &klass);
  check_jvmti_error(jvmti_env, err, "GetMethodDeclaringClass failed\n");
  if (JVMTI_ERROR_NONE != err) {
    return;
  }

  err = jvmti_env->GetClassSignature(klass, classSignature.strAddress(), classGeneric.strAddress());
  check_jvmti_error(jvmti_env, err, "GetClassSignature failed\n");
  if (JVMTI_ERROR_NONE != err) {
    return;
  }

  fprintf(stderr,"cbNativeMethodBind: %s::%s old (%p) new: (%p)\n",
    classSignature.str(),
    methodName.str(),
    address,
    *new_address_ptr);

//    fprintf(stderr,"cbNativeMethodBind: %s\n", methodName);
//
//    if (NULL == methodName || strcmp(methodName, "allocateMemory")!=0) {
//      exit_critical_section(jvmti_env);
//      return;
//    }
}


/*
 * Callback that is notified when our agent is loaded. Registers for event
 * notifications.
 */
JNIEXPORT jint JNICALL Agent_OnLoad(JavaVM *jvm, char *options,
    void *reserved) {
  jvmtiError error;
  jint res;
  jvmtiEventCallbacks callbacks;
  jvmtiEnv *jvmti_env = NULL;
  jvmtiCapabilities caps;

  memset((void*) &gdata, 0, sizeof(gdata));

  //save jvmti_env for later
  gdata.jvm = jvm;

  res = jvm->GetEnv((void **) &jvmti_env, JVMTI_VERSION_1_0);
  gdata.jvmti = jvmti_env;

  if (res != JNI_OK || jvmti_env == NULL) {
    /* This means that the VM was unable to obtain this version of the
     *   JVMTI interface, this is a fatal error.
     */
    printf("ERROR: Unable to access JVMTI Version 1 (0x%x),"
        " is your J2SE a 1.5 or newer version?"
        " JNIEnv's GetEnv() returned %d\n", JVMTI_VERSION_1, res);
  }

  //Register our capabilities
  memset(&caps, 0, sizeof(jvmtiCapabilities));
 caps.can_generate_method_entry_events = 1;
//  caps.can_generate_method_exit_events = 1;
  caps.can_generate_native_method_bind_events = 1;

  error = jvmti_env->AddCapabilities(&caps);
  check_jvmti_error(jvmti_env, error,
      "Unable to get necessary JVMTI capabilities.");

  //Register callbacks
  (void) memset(&callbacks, 0, sizeof(callbacks));
 callbacks.MethodEntry = &cbMethodEntry;
//  callbacks.MethodExit =  &cbMethodExit;
  callbacks.NativeMethodBind =  &cbNativeMethodBind;

  error = jvmti_env->SetEventCallbacks(&callbacks, (jint) sizeof(callbacks));
  check_jvmti_error(jvmti_env, error, "Cannot set jvmti callbacks");

  //Register for events
  error = jvmti_env->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_START,
      (jthread) NULL);
  check_jvmti_error(jvmti_env, error, "Cannot set vmstart event notification");
  
  error = jvmti_env->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_INIT,
      (jthread) NULL);
  check_jvmti_error(jvmti_env, error, "Cannot set vminit event notification");

  error = jvmti_env->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_VM_DEATH,
      (jthread) NULL);
  check_jvmti_error(jvmti_env, error, "Cannot set vmdeath event notification");

  error = jvmti_env->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_METHOD_ENTRY,
      (jthread) NULL);
  check_jvmti_error(jvmti_env, error, "Cannot set MethodEntry event notification");

  // error = jvmti_env->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_METHOD_EXIT,
  //     (jthread) NULL);
  // check_jvmti_error(jvmti_env, error, "Cannot set MethodExit event notification");

   error = jvmti_env->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_NATIVE_METHOD_BIND,
       (jthread) NULL);
   check_jvmti_error(jvmti_env, error, "Cannot set NativeMethodBind event notification");



  //Set up a few locks
  error = jvmti_env->CreateRawMonitor("agent data", &(gdata.lock));
  check_jvmti_error(jvmti_env, error, "Cannot create raw monitor");

  return JNI_OK;
}
