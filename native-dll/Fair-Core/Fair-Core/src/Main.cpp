
#include <Windows.h>
#include "inject/game_hook.h"
#include <string>
#include <tlhelp32.h>
#include <iostream>
#include "socket/localserver.h"
#include "ui/panel.h"
int main() {

    localserver::init();
    if (localserver::check_port(1214)) {
        localserver::send_to_java("reconnect_gui");
        panel::currentPage = panel::Page::Main;
    }
    else {
        panel::currentPage = panel::Page::Login;
    }
    panel::currentPage = panel::Page::Login;
    gui main_gui = gui();
    main_gui.init();
    FreeConsole();
    ExitProcess(0);

	return 0;
}