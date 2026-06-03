package com.fair.preload;

import sun.misc.Unsafe;

import java.io.BufferedReader;
import java.io.InputStreamReader;
import java.io.PrintWriter;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.net.Socket;
import java.util.function.Consumer;
//此类只用于加载自身jar class
//注入Core.dll时会先加载此类 此类保存在Core里 在这里改完注入完不生效。

//registerNatives用于注册NativeBridge的所有native方法
public class Preloader extends Thread {
    private static Consumer<String> messageHandler;
    private static Socket socket;
    private static PrintWriter out;
    private static BufferedReader in;
    private static Thread listenerThread;
    public static void setMessageHandler(Consumer<String> handler) {
        messageHandler = handler;
    }
    public static void startListening() {
        if (listenerThread != null && listenerThread.isAlive()) {
            return;
        }

        if (in == null) {
            return;
        }
        listenerThread = new Thread(() -> {
            try {
                String line;
                while ((line = in.readLine()) != null) {
                    if (messageHandler != null) {
                        messageHandler.accept(line);
                    }
                }
            } catch (Exception e) {

            } finally {
                try { if (socket != null) socket.close(); } catch (Exception ignored) {}
                socket = null;
                out = null;
                in = null;
            }
        });
        listenerThread.setDaemon(true);  // 主线程退出时自动结束
        listenerThread.start();
    }
    public static boolean connect(String host, int port) {
        if (socket != null && socket.isConnected() && !socket.isClosed()) {
            return true;
        }
        try {
            socket = new Socket(host, port);
            out = new PrintWriter(socket.getOutputStream(), true);
            in = new BufferedReader(new InputStreamReader(socket.getInputStream()));
            //startListening();
            return true;
        }catch (Exception e) {
            e.printStackTrace();
        }
        return false;
    }

    public static String sendAndWait(String message) {
        if (out != null) {
            try {
                out.println(message);
                return in.readLine();
            } catch (Exception e) {
                e.printStackTrace();
                return null;
            }
        }
        return "";
    }
    public static void send(String message) {
        if (out != null) {
            out.println(message);
        }
    }
    public static void cleanup() {
        try {
            if (socket != null) {
                socket.close();
            }
        } catch (Exception ignored) {}

        socket = null;
        out = null;
        in = null;
        listenerThread = null;

    }
    public static String MAIN_PATH = "C:\\NCD";

    public static String CORE_DLL = "Core.dll";
    public byte[][] classes;
    public static Class<?> mainClazz = null;
    public ClassLoader classLoader;
    public static void run(byte[][] classes, ClassLoader classLoader) {
        Preloader agentNative = new Preloader();
        agentNative.classLoader = classLoader;
        agentNative.classes = classes;
        agentNative.start();
    }
    public static byte[][] getByteArray(int size) {
        return new byte[size][];
    }
    public static boolean reconnect(String host, int port) {
//        try {
//            cleanup();
//        } catch (Exception ignored) {}

        if (connect(host, port)) {
            startListening();
            return true;
        }

        return false;
    }
    @Override
    public void run() {
        boolean connected = connect("127.0.0.1", 5555);
        if(connected) {
            MAIN_PATH = sendAndWait("run!");
            CORE_DLL = sendAndWait("ask_dll_name");
            startListening();
            send("path " + MAIN_PATH + " DLL " + CORE_DLL);

        }


        System.load(MAIN_PATH + "\\" + CORE_DLL);
        log("DIRS " + MAIN_PATH + " " + CORE_DLL);
        try {
            Class<?> unsafeClass = Class.forName("sun.misc.Unsafe");
            Field field = unsafeClass.getDeclaredField("theUnsafe");
            field.setAccessible(true);
            Unsafe unsafe = (Unsafe) field.get(null);
            Module baseModule = Object.class.getModule();
            Class<?> currentClass = Preloader.class;
            long addr = unsafe.objectFieldOffset(Class.class.getDeclaredField("module"));
            unsafe.getAndSetObject(currentClass, addr, baseModule);
            this.setContextClassLoader(classLoader);

        } catch (Exception e) {
            e.printStackTrace();
        }
        log("[Loader] Thread Run");
        send("[Loader] Thread Run");

        for (byte[] classByte : classes) {
            Class<?> clazz = defineClass(classByte);

            if (clazz == null) {
                log("[Loader] skipped 1 class");
                continue;
            }

            if (clazz.getName().contains("org.example.Main"))
                mainClazz = clazz;
        }
        if (mainClazz == null) {
            log("[Loader] main class null");
            return;
        }
        try {
            send("[Loader] tryInvoke");
            log("[Loader] tryInvoke");
            Method method = mainClazz.getDeclaredMethod("attach");
            method.invoke(null);
        } catch (Exception e) {
            log("[Loader] " + e.getMessage());
            e.printStackTrace();
        }
    }
    public static native void log(String string);
    public static native Class<?> defineClass(byte[] clazz);

    public static native void registerNatives(String target);

}
