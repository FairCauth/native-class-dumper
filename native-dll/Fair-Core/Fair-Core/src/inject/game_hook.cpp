#include "game_hook.h"
#include <hook/MinHook.h>
#include <jni/jni.h>
#include "..\native\native_bridge.h"
#include "..\common\jvm\jvm_context.h"
#include "..\native\preload\preloader.h"
#include "..\common\utils.h"
typedef void(*Java_org_lwjgl_system_JNI_callP__J)(JNIEnv* env, jclass clazz, jlong lVar);
Java_org_lwjgl_system_JNI_callP__J original_nglFlush = NULL;
static LPVOID target = nullptr;
/// <summary>
/// 程序主入口！
/// </summary>
/// <param name="env"></param>
void Entry(JNIEnv* env, JavaVM* jvm) {
    //保存jvm状态 注入的时候
    if(jvm == nullptr)
        env->GetJavaVM(&jvm);
    jvm_context::set_jvm(jvm);
    //patch_jvm();
    //保存classloader
    jobject classloader = utils::jni_func::get_classloader_from_name(env, "Render thread");
    if (!classloader) {
        MessageBoxA(NULL, "Classloader Not founded!", NULL, NULL);
    }
    jvm_context::set_classloader(classloader);

    //加载preloader （注入Class）
   	utils::writeLog("Hook down"); 
    preloader::define_loader(env);

}


void nglFlush_Hook(JNIEnv* env, jclass clazz, jlong lVar) {
    original_nglFlush(env, clazz, lVar);
    MH_DisableHook(target);

    Entry(env, nullptr);
    return;
}
void get_jniEnv(JavaVM* jvm, JNIEnv*& jni_env) {
    jni_env = nullptr;
    jvm->AttachCurrentThread(reinterpret_cast<void**>(&jni_env), nullptr);
    jvm->GetEnv(reinterpret_cast<void**>(&jni_env), JNI_VERSION_1_8);

    
}


JavaVM* get_jvm() {
    const auto jvm_dll = GetModuleHandleW(L"jvm.dll");
    using JNI_GetCreatedJavaVMs_t = jint(JNICALL*)(JavaVM**, jsize, jsize*);
    auto get_created_java_vms = reinterpret_cast<JNI_GetCreatedJavaVMs_t>(
        GetProcAddress(jvm_dll, "JNI_GetCreatedJavaVMs"));
    JavaVM* jvms[1];
    jsize n_vms = 1;
    get_created_java_vms(jvms, n_vms, &n_vms);

    if (n_vms == 0) {
        return nullptr;
    }

    return jvms[0];
}
bool game_hook::install_hook() {
    if (MH_Initialize() != MH_OK)
        return false;

    if (MH_CreateHookApiEx(L"lwjgl.dll",
            "Java_org_lwjgl_system_JNI_callP__J", 
            &nglFlush_Hook, 
            reinterpret_cast<LPVOID*>(&original_nglFlush), 
            &target) != MH_OK) {
        goto ATTACH;
        //MessageBox(NULL, L"Failed to create hook for lwjgl.dll!", L"A", MB_OK | MB_ICONWARNING);
        return false;
    }
    if (MH_EnableHook(target) != MH_OK) {
        MessageBox(NULL, L"Failed to enable hook!", L"A", MB_OK);
        return false;
    }

ATTACH:
    const auto jvm = get_jvm();
    if(!jvm) MessageBox(NULL, L"Cannot get jvm", L"ATTACH", MB_OK | MB_ICONWARNING);
    JNIEnv* jni_env;
    get_jniEnv(jvm, jni_env);
    Entry(jni_env, jvm);
    return true;
}


