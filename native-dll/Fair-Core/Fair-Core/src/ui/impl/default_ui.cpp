#include "default_ui.h"
#include "..\..\socket\localserver.h"
#include <json/json.h>
#include "..\..\components\imgui_components.h"
#include "..\..\ImGui\imconfig.h"
#include "..\..\ImGui\imgui.h"
#include "..\..\ImGui\imgui_impl_dx9.h"
#include "..\..\ImGui\imgui_impl_win32.h"
#include "..\..\ImGui\imgui_internal.h"
#include "..\..\ImGui\imstb_rectpack.h"
#include "..\..\ImGui\imstb_textedit.h"
#include "..\..\ImGui\imstb_truetype.h"
#include <string>
#include <iostream>
#include "..\font\fonts.h"
#include <windows.h>
#include <shobjidl.h>
static char text[1024] = "";
std::wstring SelectDumpFolderDialog()
{
    std::wstring result;

    HRESULT hrCo = CoInitializeEx(
        nullptr,
        COINIT_APARTMENTTHREADED | COINIT_DISABLE_OLE1DDE
    );

    bool needUninit = SUCCEEDED(hrCo);

    if (FAILED(hrCo) && hrCo != RPC_E_CHANGED_MODE)
    {
        return L"";
    }

    IFileDialog* dialog = nullptr;

    HRESULT hr = CoCreateInstance(
        CLSID_FileOpenDialog,
        nullptr,
        CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&dialog)
    );

    if (SUCCEEDED(hr))
    {
        DWORD options = 0;

        if (SUCCEEDED(dialog->GetOptions(&options)))
        {
            dialog->SetOptions(
                options |
                FOS_PICKFOLDERS |
                FOS_FORCEFILESYSTEM |
                FOS_PATHMUSTEXIST
            );
        }

        dialog->SetTitle(L"选择Dump保存目录");

        hr = dialog->Show(nullptr);

        if (SUCCEEDED(hr))
        {
            IShellItem* item = nullptr;

            hr = dialog->GetResult(&item);

            if (SUCCEEDED(hr))
            {
                PWSTR folderPath = nullptr;

                hr = item->GetDisplayName(SIGDN_FILESYSPATH, &folderPath);

                if (SUCCEEDED(hr))
                {
                    result = folderPath;
                    CoTaskMemFree(folderPath);
                }

                item->Release();
            }
        }

        dialog->Release();
    }

    if (needUninit)
    {
        CoUninitialize();
    }

    return result;
}
std::pair<std::string, std::string> splitByFirstSpace(const std::string& text) {
    size_t pos = text.find(' ');

    if (pos == std::string::npos) {
        return { text, "" };
    }

    return {
        text.substr(0, pos),
        text.substr(pos + 1)
    };
}
static const char* selected = nullptr;

static std::string selectedClass;
#include <map>
#include <memory>
#include <vector>
#include <sstream>
struct ClassTreeNode
{
    std::map<std::string, std::unique_ptr<ClassTreeNode>> children;

    bool isClass = false;
    std::string fullName;
};
std::string ToLowerString(std::string s)
{
    for (char& c : s)
    {
        c = (char)std::tolower((unsigned char)c);
    }

    return s;
}
static ClassTreeNode classTree;
static bool treeBuilt = false;

static size_t lastListHash = 0;
#include <mutex>

static std::mutex listMutex;
static bool hasTree = false;

// 完整列表：点击刷新后保存全部 class
static std::vector<std::string> allClassList;

// 当前显示列表：搜索后会变少
static std::vector<std::string> visibleClassList;

// 搜索框
static char classSearchText[256] = "";

// 搜索时自动展开树
static bool forceOpenTree = false;
bool ContainsIgnoreCase(const std::string& text, const std::string& keyword)
{
    if (keyword.empty())
        return true;

    std::string lowerText = ToLowerString(text);
    std::string lowerKeyword = ToLowerString(keyword);

    return lowerText.find(lowerKeyword) != std::string::npos;
}
std::vector<std::string> Split(const std::string& str, char delimiter)
{
    std::vector<std::string> result;
    std::stringstream ss(str);
    std::string item;

    while (std::getline(ss, item, delimiter))
    {
        if (!item.empty())
            result.push_back(item);
    }

    return result;
}

void InsertClassName(ClassTreeNode& root, const std::string& className)
{
    std::vector<std::string> parts = Split(className, '.');

    ClassTreeNode* current = &root;

    for (const std::string& part : parts)
    {
        if (current->children.find(part) == current->children.end())
        {
            current->children[part] = std::make_unique<ClassTreeNode>();
        }

        current = current->children[part].get();
    }

    current->isClass = true;
    current->fullName = className;
}
ClassTreeNode BuildClassTree(const std::vector<std::string>& classNames)
{
    ClassTreeNode root;

    for (const std::string& name : classNames)
    {
        InsertClassName(root, name);
    }

    return root;
}
void DrawClassTreeNode(const std::string& name, ClassTreeNode* node)
{
    ImGui::PushID(node);

    bool hasChildren = !node->children.empty();

    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_OpenOnArrow |
        ImGuiTreeNodeFlags_SpanAvailWidth;

    // 搜索后自动展开包路径
    if (forceOpenTree && hasChildren)
    {
        flags |= ImGuiTreeNodeFlags_DefaultOpen;
    }

    if (!hasChildren)
    {
        flags |= ImGuiTreeNodeFlags_Leaf |
            ImGuiTreeNodeFlags_NoTreePushOnOpen;
    }

    if (node->isClass && selectedClass == node->fullName)
    {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    bool open = ImGui::TreeNodeEx(name.c_str(), flags);

    if (ImGui::IsItemClicked())
    {
        if (node->isClass)
        {
            selectedClass = node->fullName;
        }
    }

    if (hasChildren && open)
    {
        for (auto& [childName, childNode] : node->children)
        {
            DrawClassTreeNode(childName, childNode.get());
        }

        ImGui::TreePop();
    }

    ImGui::PopID();
}


#include "..\..\common\utils.h"
#include <algorithm>
void DrawClassTreeWindow()
{
    static float leftWidth = 300.0f;

    ImVec2 avail = ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y - 30);

    float splitterWidth = 6.0f;
    float minLeftWidth = 150.0f;
    float minRightWidth = 200.0f;

    float maxLeftWidth = avail.x - splitterWidth - minRightWidth;

    if (maxLeftWidth < minLeftWidth)
        maxLeftWidth = minLeftWidth;

    if (leftWidth < minLeftWidth)
        leftWidth = minLeftWidth;

    if (leftWidth > maxLeftWidth)
        leftWidth = maxLeftWidth;

    ImGui::BeginChild("##class_tree", ImVec2(leftWidth, avail.y), true);

    if (hasTree)
    {
        for (auto& [name, node] : classTree.children)
        {
            DrawClassTreeNode(name, node.get());
        }
    }
    else
    {
        ImGui::TextDisabled("Nothing...");
    }

    ImGui::EndChild();

    ImGui::SameLine(0.0f, 0.0f);

    ImGui::InvisibleButton("##splitter", ImVec2(splitterWidth, avail.y));

    if (ImGui::IsItemHovered())
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeEW);

    if (ImGui::IsItemActive())
    {
        leftWidth += ImGui::GetIO().MouseDelta.x;

        if (leftWidth < minLeftWidth)
            leftWidth = minLeftWidth;

        if (leftWidth > maxLeftWidth)
            leftWidth = maxLeftWidth;
    }

    ImGui::SameLine(0.0f, 0.0f);
    ImGui::BeginChild("##class_info", ImVec2(0, avail.y), true);

    ImGui::Text("Classes: %d / %d",
        (int)visibleClassList.size(),
        (int)allClassList.size()
    );

    ImGui::Separator();

    ImGui::Text("Selected Class:");

    if (!selectedClass.empty())
    {
        if (ImGui::Button("Dump"))
        {
            std::wstring folderW = SelectDumpFolderDialog();
            //用户取消选择
            if (!folderW.empty())
            {
                std::string folder = utils::convert::wstring2utf8string(folderW);

                Json::Value json;
                Json::FastWriter writer;

                json["cmd"] = "dump";
                json["value"] = selectedClass;
                json["dir"] = folder;

                const std::string data = writer.write(json);
                localserver::send(data);
            }
        }
        
    }

    ImGui::EndChild();
}
void RefreshClassTree()
{
    std::vector<std::string> snapshot;

    {
        std::lock_guard<std::mutex> lock(listMutex);
        snapshot = default_ui::list_elements;
    }

    allClassList = snapshot;
    visibleClassList = allClassList;

    classTree = BuildClassTree(visibleClassList);

    selectedClass.clear();
    hasTree = true;
    forceOpenTree = false;
}
void ApplyClassSearch()
{
    std::string keyword = classSearchText;

    visibleClassList.clear();

    for (const std::string& name : allClassList)
    {
        if (ContainsIgnoreCase(name, keyword))
        {
            visibleClassList.push_back(name);
        }
    }

    classTree = BuildClassTree(visibleClassList);

    selectedClass.clear();
    hasTree = true;

    //有搜索内容时自动展开树
    forceOpenTree = !keyword.empty();
}
void default_ui::on_draw(gui& gui) {
    std::string filterText(text);
    if (ImGui::Button("Get Loaded Classes"))
    {
        Json::Value json;
        Json::FastWriter writer;

        json["cmd"] = "search";
        json["value"] = filterText;

        const std::string data = writer.write(json);
        localserver::send(data);
    }

    ImGui::Separator();

    ImGui::Text("Class Search");

    ImGui::SetNextItemWidth(260);

    bool enterPressed = ImGui::InputText(
        "##ClassSearch",
        classSearchText,
        sizeof(classSearchText),
        ImGuiInputTextFlags_EnterReturnsTrue
    );

    if (enterPressed)
    {
        ApplyClassSearch();
    }

    ImGui::SameLine();

    if (ImGui::Button("Search"))
    {
        ApplyClassSearch();
    }

    ImGui::SameLine();

    if (ImGui::Button("Clear"))
    {
        classSearchText[0] = '\0';
        ApplyClassSearch();
    }

    ImGui::SameLine();

    if (ImGui::Button("Refresh Tree"))
    {
        RefreshClassTree();
    }

    DrawClassTreeWindow();
    //if (ImGui::BeginListBox("##modules", ImVec2(gui::WINDOW_WIDTH - 100, gui::WINDOW_HEIGHT - 100))) {
    //    for (int i = 0; i < list_elements.size(); i++) {
    //        bool isSelected = (selectedModule == i);

    //        if (ImGui::Selectable(list_elements[i].c_str(), isSelected)) {
    //            selectedModule = i;
    //        }

    //        if (isSelected) {
    //            ImGui::SetItemDefaultFocus();
    //        }
    //    }

    //    ImGui::EndListBox();
    //}
    //if (default_ui::selectedModule >= 0 &&
    //    default_ui::selectedModule < default_ui::list_elements.size()) {

    //    std::string selectedName =
    //        default_ui::list_elements[default_ui::selectedModule];

    //    if (ImGui::Button("Dump")) {
    //        Json::Value json;
    //        Json::FastWriter writer;
    //        json["cmd"] = "dump";
    //        json["value"] = selectedName;
    //        const std::string data = writer.write(json);
    //        localserver::send(data);
    //    }
    //}
}
void default_ui::refresh_list() {
    RefreshClassTree();
}