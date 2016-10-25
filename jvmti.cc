#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include "jvmti.h"
#include "jni.h"


typedef struct {
  /* JVMTI Environment */
  jvmtiEnv *jvmti;
  JNIEnv * jni;
  jboolean vm_is_started;
  jboolean vmDead;

  /* Data access Lock */
  jrawMonitorID lock;
  JavaVM* jvm;
} GlobalAgentData;


GlobalAgentData data;
GlobalAgentData *gdata;


typedef jlong (*reallocateMemoryFptr)(JNIEnv *env, jobject unsafe, jlong addr, jlong size);
typedef jlong (*allocateMemoryFptr)(JNIEnv *env, jobject unsafe, jlong size);

reallocateMemoryFptr oldReallocateMemory = NULL;
allocateMemoryFptr oldAllocateMemory = NULL;

jlong new_Unsafe_ReallocateMemory(JNIEnv *env, jobject unsafe, jlong addr, jlong size)
{
  return (*oldReallocateMemory)(env, unsafe, addr, size);
}


jlong new_Unsafe_AllocateMemory(JNIEnv *env, jobject unsafe, jlong size)
{
  return (*oldAllocateMemory)(env, unsafe, size);
}

static void release(jvmtiEnv *jvmti_env, char **p) {
  if (NULL == p) {
    return;
  }

  if (NULL == *p) {
    return;
  }

  jvmti_env->Deallocate((unsigned char *)*p);
  *p = NULL;
}

static void check_jvmti_error(jvmtiEnv *jvmti_env, jvmtiError errnum,
    const char *str) {
  if (errnum != JVMTI_ERROR_NONE) {
    char *errnum_str;

    errnum_str = NULL;
    jvmti_env->GetErrorName(errnum, &errnum_str);

    fprintf(stderr, "ERROR: JVMTI: %d(%s): %s\n", errnum,
        (errnum_str == NULL ? "Unknown" : errnum_str),
        (str == NULL ? "" : str));

    release(jvmti_env, &errnum_str);
  }
}

/* Enter a critical section by doing a JVMTI Raw Monitor Enter */
static void enter_critical_section(jvmtiEnv *jvmti_env) {
  jvmtiError error;

  error = jvmti_env->RawMonitorEnter(gdata->lock);
  check_jvmti_error(jvmti_env, error, "Cannot enter with raw monitor");
}

/* Exit a critical section by doing a JVMTI Raw Monitor Exit */
static void exit_critical_section(jvmtiEnv *jvmti_env) {
  jvmtiError error;

  error = jvmti_env->RawMonitorExit(gdata->lock);
  check_jvmti_error(jvmti_env, error, "Cannot exit with raw monitor");
}

JNICALL void cbMethodEntry
    (jvmtiEnv *jvmti_env,
     JNIEnv* jni_env,
     jthread thread,
     jmethodID method)
{
  enter_critical_section(jvmti_env);

  char *methodName = NULL;
  char *signaturePtr = NULL;
  char *genericPtr = NULL;
  jvmtiError err = JVMTI_ERROR_NONE;

  err = jvmti_env->GetMethodName(method, &methodName, &signaturePtr, &genericPtr);
  if (err != JVMTI_ERROR_NONE) {
    check_jvmti_error(jvmti_env, err, "GetMethodName failed\n");
    exit_critical_section(jvmti_env);
    return;
  }

  if (NULL == methodName || strcmp(methodName, "allocateMemory")!=0) {
    exit_critical_section(jvmti_env);
    return;
  }

  fprintf(stderr,"MethodEntry: %s\n", methodName);

  release(jvmti_env, &methodName);
  release(jvmti_env, &signaturePtr);
  release(jvmti_env, &genericPtr);

  jvmtiFrameInfo frames[5] = {0,};
  jint count = 0;

  err = jvmti_env->GetStackTrace(NULL, 0, 5, frames, &count);
  check_jvmti_error(jvmti_env, err, "GetStackTrace failed\n");

  if (err == JVMTI_ERROR_NONE && count >= 1) {
    int i=0;
    for (;i<count;i++) {
      jclass klass = NULL;
      err = jvmti_env->GetMethodDeclaringClass(frames[i].method, &klass);
      check_jvmti_error(jvmti_env, err, "GetMethodDeclaringClass failed\n");

      if (err == JVMTI_ERROR_NONE) {
        char *framesMethodName = NULL;
        char *framesSignaturePtr = NULL;
        char *framesGenericPtr = NULL;

        err = jvmti_env->GetMethodName(frames[i].method, &framesMethodName, &framesSignaturePtr, &framesGenericPtr);
        check_jvmti_error(jvmti_env, err, "GetMethodName2 failed\n");

        if (err == JVMTI_ERROR_NONE) {
          char *classSignature = NULL;
          char *classGeneric = NULL;

          err = jvmti_env->GetClassSignature(klass, &classSignature, &classGeneric);
          check_jvmti_error(jvmti_env, err, "GetClassSignature failed\n");

          jint paramsSize = 0;
          if (err == JVMTI_ERROR_NONE) {
            err = jvmti_env->GetArgumentsSize(frames[i].method, &paramsSize);
            check_jvmti_error(jvmti_env, err, "GetArgumentsSize failed\n");

          }
          fprintf(stderr,"[%d] %s::%s  (%d)\n", i, classSignature, framesMethodName, paramsSize);

          release(jvmti_env, &classSignature);
          release(jvmti_env, &classGeneric);
        }

        release(jvmti_env, &framesMethodName);
        release(jvmti_env, &framesSignaturePtr);
        release(jvmti_env, &framesGenericPtr);
      }
    }
  }

  exit_critical_section(jvmti_env);
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
    enter_critical_section(jvmti_env);

    // default to nothing.
    *new_address_ptr = address;

    char *methodName = NULL;
    char *signaturePtr = NULL;
    char *genericPtr = NULL;
    jvmtiError err = JVMTI_ERROR_NONE;

    err = jvmti_env->GetMethodName(method, &methodName, &signaturePtr, &genericPtr);
    if (err != JVMTI_ERROR_NONE) {
      check_jvmti_error(jvmti_env, err, "GetMethodName failed\n");
      exit_critical_section(jvmti_env);
      return;
    }

    bool isAllocate = (0 == strcmp(methodName,"allocateMemory"));
    bool isReAllocate = (0 == strcmp(methodName, "reallocateMemory"));

    if (!isAllocate && !isReAllocate) {
      exit_critical_section(jvmti_env);
      return;
    }

    jclass klass = NULL;
    char *classSignature = NULL;
    char *classGeneric = NULL;

    err = jvmti_env->GetMethodDeclaringClass(method, &klass);
    check_jvmti_error(jvmti_env, err, "GetMethodDeclaringClass failed\n");

    err = jvmti_env->GetClassSignature(klass, &classSignature, &classGeneric);
    check_jvmti_error(jvmti_env, err, "GetClassSignature failed\n");

    fprintf(stderr,"cbNativeMethodBind: %s::%s old (%p) new: (%p)\n", classSignature, methodName, address, *new_address_ptr);

//    fprintf(stderr,"cbNativeMethodBind: %s\n", methodName);
//
//    if (NULL == methodName || strcmp(methodName, "allocateMemory")!=0) {
//      exit_critical_section(jvmti_env);
//      return;
//    }

    release(jvmti_env, &methodName);
    release(jvmti_env, &signaturePtr);
    release(jvmti_env, &genericPtr);

    release(jvmti_env, &classSignature);
    release(jvmti_env, &classGeneric);

    exit_critical_section(jvmti_env);
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

  gdata = &data;
  memset((void*) gdata, 0, sizeof(data));

  //save jvmti_env for later
  gdata->jvm = jvm;

  res = jvm->GetEnv((void **) &jvmti_env, JVMTI_VERSION_1_0);
  gdata->jvmti = jvmti_env;

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
//  caps.can_generate_method_entry_events = 1;
//  caps.can_generate_method_exit_events = 1;
  caps.can_generate_native_method_bind_events = 1;

  error = jvmti_env->AddCapabilities(&caps);
  check_jvmti_error(jvmti_env, error,
      "Unable to get necessary JVMTI capabilities.");

  //Register callbacks
  (void) memset(&callbacks, 0, sizeof(callbacks));
//  callbacks.MethodEntry = &cbMethodEntry;
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

  error = jvmti_env->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_METHOD_EXIT,
      (jthread) NULL);
  check_jvmti_error(jvmti_env, error, "Cannot set MethodExit event notification");

   error = jvmti_env->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_NATIVE_METHOD_BIND,
       (jthread) NULL);
   check_jvmti_error(jvmti_env, error, "Cannot set NativeMethodBind event notification");



  //Set up a few locks
  error = jvmti_env->CreateRawMonitor("agent data", &(gdata->lock));
  check_jvmti_error(jvmti_env, error, "Cannot create raw monitor");

  return JNI_OK;
}
