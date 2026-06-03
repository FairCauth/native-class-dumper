package org.example;

import com.fair.preload.Preloader;
import com.google.gson.*;

import java.io.BufferedReader;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStreamReader;
import java.net.ServerSocket;
import java.net.Socket;
import java.nio.file.Files;
import java.nio.file.Path;

public class ExternalGui {
    private static final Gson gson = new Gson();
    public ExternalGui() {
        Preloader.setMessageHandler(this::onMessage);
        startServer(1214);
    }
    private ServerSocket serverSocket;
    public void stopServer() {
        try {
            if (serverSocket != null) serverSocket.close();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }
    public void startServer(int port) {
        new Thread(() -> {
            try {
                serverSocket = new ServerSocket(port);
                System.out.println("Java已启动，监听端口: " + port);
                while (true) {
                    Socket client = serverSocket.accept();
                    System.out.println("客户端连接: "
                            + client.getInetAddress()
                            + ":" + client.getPort());
                    new Thread(() -> handleClient(client)).start();
                }

            } catch (Exception e) {
                e.printStackTrace();
            }
        }).start();
    }

    private void handleClient(Socket client) {

        try {

            BufferedReader reader = new BufferedReader(
                    new InputStreamReader(client.getInputStream())
            );
            String line;
            while ((line = reader.readLine()) != null) {
                System.out.println("收到消息: " + line);
                onClientMessage(line);
            }
        } catch (Exception e) {
            System.out.println("客户端断开");
        }
    }
    private void onClientMessage(String message) {
        if(message.equals("reconnect_gui")) {
            System.out.println("reconnect_gui");
            Preloader.reconnect("127.0.0.1", 5555);
//            registerMain();
            return;
        }

    }

    public void onMessage(String message) {
        if(!isJson(message)) return;
        JsonObject json = gson.fromJson(message, JsonObject.class);
        String cmd = json.get("cmd").getAsString();
        String value = json.get("value").getAsString();
        if(cmd.equals("dump")) {
            String dir = json.get("dir").getAsString();
            try {
                Class<?> clazz = Class.forName(value);
                byte[] data = CoreNative.getClassBytes(clazz);

                Files.write(Path.of(dir + "\\" + clazz.getName() + ".class"), data);
            }catch (Exception e) {
                 e.printStackTrace();
            }

        }
        if(cmd.equals("search")) {
            JsonObject jsonModule = new JsonObject();
            jsonModule.addProperty("type", "update_list");

            JsonArray array = new JsonArray();
            for (String loadedClass : CoreNative.getLoadedClasses()) {
                if(loadedClass.startsWith(value))
                    array.add(loadedClass);
            }

            jsonModule.add("values", array);
            String text = gson.toJson(jsonModule);
            Preloader.send(text);
        }
        System.out.println(cmd + " " +value);
    }
    public boolean isJson(String str) {
        if (str == null || str.isBlank()) return false;
        try {
            JsonElement el = JsonParser.parseString(str);
            return el.isJsonObject() || el.isJsonArray();
        } catch (JsonSyntaxException e) {
            return false;
        }
    }
}
