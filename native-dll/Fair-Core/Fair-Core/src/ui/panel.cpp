#include "panel.h"
#include "..\ImGui\imconfig.h"
#include "..\ImGui\imgui.h"
#include "..\ImGui\imgui_impl_dx9.h"
#include "..\ImGui\imgui_impl_win32.h"
#include "..\ImGui\imgui_internal.h"
#include "..\ImGui\imstb_rectpack.h"
#include "..\ImGui\imstb_textedit.h"
#include "..\ImGui\imstb_truetype.h"
#include <string>
#include <iostream>


#include "..\socket\localserver.h"
std::string right_of(const std::string& s, const std::string& delimiter) {
    size_t pos = s.find(delimiter);
    if (pos == std::string::npos) return "";
    return s.substr(pos + delimiter.size());
}

#include "impl/default_ui.h"
int max = 0;
#include "..\dist\json\json.h"

#include "..\common\utils.h"
std::string panel::from_client(std::string msg) {
    //std::cout << "[recv] " << msg << std::endl;
    bool printLog = true;
    if (msg.find("run!") != std::string::npos) {
        std::cout << "ÊÕµ½Æô¶¯ÃüÁî" << std::endl;
        
        return utils::others::get_self_path();
    }
    if (msg.find("ask_dll_name") != std::string::npos) {
        std::cout << "ask_dll_name" << std::endl;
        return utils::others::get_self_name();
    }
    std::string result = right_of(msg, "start transformer ");
    if (result != "") {
        max = std::stoi(result);
    }

    
    if (msg.find("init ok") != std::string::npos) {
        panel::currentPage = panel::Page::Main;
        //localserver::shutdown();
    }

    if (msg.find("{") != std::string::npos) {
        printLog = false;
        Json::Reader reader;
        Json::Value root;
        bool success = reader.parse(msg, root);
        if (!success) {
            std::cout << "Error json " << msg << std::endl;
        }
        std::string type = root["type"].asString();

        if (type == "update_list") {
            const Json::Value& modules = root["values"];
            default_ui::list_elements.clear();

            if (modules.isArray()) {
                for (const Json::Value& item : modules) {
                    std::string moduleName = item.asString();
                    default_ui::list_elements.push_back(moduleName);
                    //std::cout << "value: " << moduleName << std::endl;
                }
            }
            default_ui::refresh_list();
        }


    }

    if (printLog) {
        std::cout << "[recv] " << msg << std::endl;
    }
    return "";
}
#include "..\common\utils.h"
#include <vector>
#include <psapi.h>
#include <locale>
#include <codecvt>
#include <tlhelp32.h>
#include <comdef.h>
static bool show_list = false;
static int selected = -1;
struct JavaProcess {
    DWORD pid;
    std::string windowTitle;
};
static std::vector<JavaProcess> java_processes;
struct EnumData {
    DWORD pid;
    std::string title;
};

static BOOL CALLBACK FindWindowByPid(HWND hwnd, LPARAM lParam) {
    auto* data = (EnumData*)lParam;
    DWORD pid;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == data->pid) {
        char title[256];
        GetWindowTextA(hwnd, title, sizeof(title));
        if (strlen(title) > 0) {
            data->title = title;
            return FALSE;
        }
    }
    return TRUE;
}
void RefreshJavaList() {
    java_processes.clear();

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32W pe{};
    pe.dwSize = sizeof(pe);

    if (Process32FirstW(snapshot, &pe)) {
        do {
            std::wstring name(pe.szExeFile);
            if (name == L"java.exe" || name == L"javaw.exe") {
                EnumData data{ pe.th32ProcessID, "" };
                EnumWindows(FindWindowByPid, (LPARAM)&data);
                if (data.title.empty())
                    data.title = "PID " + std::to_string(pe.th32ProcessID);

                java_processes.push_back({ pe.th32ProcessID, data.title });
            }
        } while (Process32NextW(snapshot, &pe));
    }

    CloseHandle(snapshot);
}
#include "..\components\imgui_components.h"
static char text[1024] = "";
static char pwd[1024] = "";
static bool remember;
void draw_login_gui(gui& gui) {
    if (ImGui::BeginTabBar("Bar1")) {
        if (ImGui::BeginTabItem("Login")) {
            std::string str = "Test Client";
            ImFont* glitchText = gui.glitchText;
            ImVec2 size = glitchText->CalcTextSizeA(
                glitchText->FontSize,
                FLT_MAX,
                0.0f,
                str.c_str()
            );
           
            float child_width = 400, child_height = 200;

            ImGui::SetCursorPos({ gui::WINDOW_WIDTH / 2.0f - child_width / 2.0f, gui::WINDOW_HEIGHT / 2.0f - child_height / 2.0f });

            ImGui::BeginChild("##A", { child_width, child_height }, false, ImGuiWindowFlags_NoScrollbar);
            for (int i = 0; i < java_processes.size(); i++) {
                std::string label = "[" + std::to_string(java_processes[i].pid) + "] " + java_processes[i].windowTitle;
                if (ImGui::Selectable(label.c_str(), selected == i))
                    selected = i;
            }
            if (ImGui::Button("Refresh", { 100, 40 }))
            {
                std::cout << utils::others::get_self_path() << " " << utils::others::get_self_name() << std::endl;
                RefreshJavaList();
                selected = -1;
            }


                

                if (selected >= 0 && ImGui::Button("OK", { 100, 30 })) {
                    DWORD pid = java_processes[selected].pid;
                    const char* dllPath = (utils::others::get_self_path() + "\\" + utils::others::get_self_name()).c_str();
                    localserver::init();
                    Sleep(1000);
                    utils::others::inject_dll(pid, dllPath);
                    max = 0;
                }


            ImGui::EndChild();


            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Register")) {
         
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

}


void draw_main_gui(gui& gui) {
    
}


void panel::on_draw(gui& gui) {

    switch (currentPage)
    {
    case Login:
        //default_ui::on_draw(gui);
        draw_login_gui(gui);
        //music_ui::on_draw(gui);
        //vapelite_ui::on_draw(gui);
        break;
    case Main:
        default_ui::on_draw(gui);
        //draw_main_gui(gui);
        break;
    }


}

