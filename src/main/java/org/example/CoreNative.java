package org.example;

import com.fair.preload.Preloader;

public class CoreNative {
    public static void init() {
        System.load(Preloader.MAIN_PATH + "\\" + Preloader.CORE_DLL);
    }
    public static native boolean startup();
    public static native int redefineClasses(Class<?> targetClass, byte[] newClassBytes);
    public static native byte[] getClassBytes(Class<?> clazz);
    public static native String[] getLoadedClasses();
}
