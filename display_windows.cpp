#include "imgui.h"
#include <cppconn/connection.h>
#include <cppconn/driver.h>
#include "includes/jsonstuff.h"
#include "includes/display_windows.h"

void display_connection_add(tasker::DisplayWindow& stage) {
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;
    ImGui::SetNextWindowSize(ImVec2(500, 300));
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + (viewport->Size.x / 2) - 250, viewport->Pos.y + (viewport->Size.y / 2) - 250));
    ImGui::SetWindowFocus("Add database");
    sql::Connection* connection = nullptr;

    if(ImGui::Begin("Add database", nullptr, flags)) {
        ImGui::Text("Database IP: "); ImGui::SameLine();
        static char ip[128] = "";
        ImGui::PushItemWidth(250);
        ImGui::SetCursorPosX(150);
        ImGui::PushID(0);
        ImGui::InputText("", ip, IM_ARRAYSIZE(ip));
        ImGui::PopID();

        ImGui::Text("Port:"); ImGui::SameLine();
        static int port = 3306;
        ImGui::SetCursorPosX(150);
        ImGui::PushID(1);
        ImGui::InputInt("", &port, 0, 0);
        ImGui::PopID();
        if(port < 0) port = 0;
        else if(port > 65535) port = 65535;

        ImGui::Text("Username: "); ImGui::SameLine();
        static char username[128];
        ImGui::SetCursorPosX(150);
        ImGui::PushID(2);
        ImGui::InputText("", username, IM_ARRAYSIZE(username));
        ImGui::PopID();

        static bool show_pass = false;

        ImGui::Text("Password: "); ImGui::SameLine();
        static char password[128];
        ImGui::SetCursorPosX(150);
        ImGui::PushID(3);
        ImGui::InputText("", password, IM_ARRAYSIZE(password), (show_pass ? 0 : ImGuiInputTextFlags_Password) | ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::PopID();

        ImGui::SetCursorPosX(150);
        ImGui::Checkbox("Show", &show_pass);

        ImGui::NewLine();
        ImGui::SetCursorPosX(200);
        bool connect_ready = ImGui::Button("Connect", ImVec2(100, 25));
        static bool show_login_error = false;

        if(connect_ready && !(ip[0] == '\0' || username[0] == '\0' || password[0] == '\0')) {
            sql::Connection* con;
            try {
                connect_ready = false;
                sql::Connection* con = get_driver_instance()->connect("tcp://" + ((std::string) ip) + ":" + std::to_string(port), (std::string) username, (std::string) password);
                tasker::add_json_connection(tasker::json_sql_connection{(std::string) ip, port, (std::string) username, (std::string) password, ""});
                stage = tasker::DisplayWindow::pick_workspace;
                show_login_error = false;
                delete con;
            } catch(sql::SQLException& e) {
                //delete con;
                show_login_error = true;
            }
        }

        if(show_login_error) {
            ImGui::NewLine();
            ImGui::GetStyle().Colors[ImGuiCol_Text] = ImVec4(1, 0, 0, 1);
            ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Could not connect! Check login info!").x) / 2);
            ImGui::Text("Could not connect! Check login info!");
            ImGui::GetStyle().Colors[ImGuiCol_Text] = ImVec4(1, 1, 1, 1);
        }

        ImGui::End();
    }
}