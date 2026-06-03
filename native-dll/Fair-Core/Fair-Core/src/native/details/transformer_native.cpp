#include "transformer_native.h"
#include "natives.h"
#include <sstream>
#include <iostream>
jboolean JNICALL native_functions::startup(JNIEnv* env, jclass clazz) {
    jvmtiCapabilities capabilities;
    memset(&capabilities, 0, sizeof(jvmtiCapabilities));
    capabilities.can_retransform_classes = true;
    capabilities.can_retransform_any_class = true;
    capabilities.can_redefine_any_class = true;
    capabilities.can_redefine_classes = true;
    capabilities.can_generate_all_class_hook_events = true;

    jvmtiError capError = jvm_context::get_jvmTi()->AddCapabilities(&capabilities);
    if (capError != JVMTI_ERROR_NONE)
        return false;

    jvmtiEventCallbacks callbacks;
    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.ClassFileLoadHook = &classFileLoadHook;
    jvm_context::get_jvmTi()->SetEventCallbacks(&callbacks, sizeof(callbacks));
    jvm_context::get_jvmTi()->SetEventNotificationMode(JVMTI_ENABLE, JVMTI_EVENT_CLASS_FILE_LOAD_HOOK, NULL);
    return true;
}

jint JNICALL native_functions::redefineClasses(JNIEnv* env, jclass cls, jclass targetClass, jbyteArray newClassBytes) {

    jsize length = env->GetArrayLength(newClassBytes);
    jbyte* bytes = env->GetByteArrayElements(newClassBytes, NULL);
    jvmtiClassDefinition classDef;
    memset(&classDef, 0, sizeof(jvmtiClassDefinition));
    classDef.klass = targetClass;
    classDef.class_byte_count = length;
    classDef.class_bytes = (unsigned char*)bytes;
    jvmtiError error = jvm_context::get_jvmTi()->RedefineClasses(1, &classDef);
    //sendMsg("Redefine class ");
    //JVMTI_ERROR_UNSUPPORTED_REDEFINITION_SCHEMA_CHANGED
    env->ReleaseByteArrayElements(newClassBytes, bytes, JNI_ABORT);
    

    return (jint)error;
}

jbyteArray JNICALL native_functions::getClassBytes(JNIEnv* env, jclass cls, jclass targetClass) {

    targetClazz = targetClass;
    class_bytes = nullptr;
    jclass* classes = (jclass*)allocate(sizeof(jclass));
    classes[0] = targetClass;
    jvm_context::get_jvmTi()->RetransformClasses(1, classes);
    while (!class_bytes)
    {
        continue;
    }
    jbyteArray outputArray = env->NewByteArray(class_bytes_len);
    env->SetByteArrayRegion(outputArray, 0, class_bytes_len, (jbyte*)class_bytes);
    jvm_context::get_jvmTi()->Deallocate((unsigned char*)classes);
    return outputArray;
}
static bool ToNormalClassName(const char* sig, std::string& out) {
    if (!sig) return false;

    std::string s = sig;

    // 过滤数组类
    if (!s.empty() && s[0] == '[') {
        return false;
    }

    // 只要普通对象类：Ljava/lang/String;
    if (s.size() >= 2 && s[0] == 'L' && s.back() == ';') {
        out = s.substr(1, s.size() - 2);

        for (char& c : out) {
            if (c == '/') {
                c = '.';
            }
        }

        return true;
    }

    return false;
}
#include <vector>
jobjectArray  JNICALL native_functions::getLoadedClasses(JNIEnv* env, jclass cls) {
    jint classCount = 0;
    jclass* classes = nullptr;

    jvmtiError err = jvm_context::get_jvmTi()->GetLoadedClasses(&classCount, &classes);
    if (err != JVMTI_ERROR_NONE || !classes) {
        jclass stringClass = env->FindClass("java/lang/String");
        return env->NewObjectArray(0, stringClass, nullptr);
    }

    std::vector<std::string> names;

    for (int i = 0; i < classCount; i++) {
        char* sig = nullptr;
        char* generic = nullptr;

        if (jvm_context::get_jvmTi()->GetClassSignature(classes[i], &sig, &generic) == JVMTI_ERROR_NONE && sig) {
            std::string name;

            if (ToNormalClassName(sig, name)) {
                names.push_back(name);
            }
        }

        if (sig) {
            jvm_context::get_jvmTi()->Deallocate((unsigned char*)sig);
        }

        if (generic) {
            jvm_context::get_jvmTi()->Deallocate((unsigned char*)generic);
        }
    }

    jvm_context::get_jvmTi()->Deallocate((unsigned char*)classes);

    jclass stringClass = env->FindClass("java/lang/String");

    jobjectArray result = env->NewObjectArray(
        static_cast<jsize>(names.size()),
        stringClass,
        nullptr
    );

    for (int i = 0; i < names.size(); i++) {
        jstring str = env->NewStringUTF(names[i].c_str());
        env->SetObjectArrayElement(result, i, str);
        env->DeleteLocalRef(str);
    }

    return result;
}