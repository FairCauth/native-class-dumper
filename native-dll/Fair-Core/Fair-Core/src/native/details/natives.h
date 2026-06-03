#pragma once
#include "jni/jni.h"
namespace native_functions {
	jobjectArray JNICALL getLoadedClasses(JNIEnv*, jclass);
	jint JNICALL redefineClasses(JNIEnv*, jclass, jclass, jbyteArray);
	jbyteArray JNICALL getClassBytes(JNIEnv*, jclass, jclass);
	jboolean JNICALL startup(JNIEnv*, jclass);
	void JNICALL test(JNIEnv*, jclass);


	void JNICALL initSkia(JNIEnv*, jclass);

}