package org.example;

import com.fair.preload.Preloader;

import java.util.Arrays;

public class Main {
    public static ExternalGui externalGui;
    public static void attach() {
        CoreNative.init();
        Preloader.registerNatives("Lorg/example/CoreNative;");
        CoreNative.startup();

        Preloader.send("init ok");
        externalGui = new ExternalGui();
        System.out.println("111111111111111111");

    }
    public static void main(String[] args) {
        boolean connected = Preloader.connect("127.0.0.1", 5555);
        if(connected) {
            Preloader.startListening();
            attach();
        }
    }
}