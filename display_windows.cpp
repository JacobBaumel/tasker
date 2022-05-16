#include "includes/display_windows.h"

#include <cppconn/connection.h>
#include <cppconn/driver.h>

#include "Colors.h"
#include "imgui.h"
#include "includes/jsonstuff.h"

void display_connection_add(bool& add_connection) {
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;
    ImGui::SetNextWindowSize(ImVec2(500, 300));
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + (viewport->Size.x / 2) - 250, viewport->Pos.y + (viewport->Size.y / 2) - 250));
    ImGui::SetWindowFocus("Add database");
    sql::Connection* connection = nullptr;
    static bool refresh = false;
    static tasker::json_sql_connection_array* connections;
    static int current_connection = 0;
    static std::string* connection_options = NULL;
    static const char* current = NULL;

    if (ImGui::Begin("Add new workspace", &add_connection, flags)) {
        if (ImGui::BeginTabBar("AddTab")) {
            if(!refresh) {
                refresh = true;
                connections = tasker::get_json_connections();
                if(connection_options != NULL) delete[] connection_options;

                connection_options = new std::string[connections->length];

                for(int i = 0; i < connections->length; i++) {
                    connection_options[i] = (connections->connections + i)->ip;
                    connection_options[i].append(":").append(std::to_string((connections->connections + i)->port));
                }

                current = connection_options[0].c_str();   
            }

            if(ImGui::BeginTabItem("Create New")) {
                ImGui::Text("Connection: ");
                ImGui::SameLine();
                if(ImGui::BeginCombo("##connection", current)) {
                    for(int i = 0; i < connections->length; i++) {
                        bool selected = current == connection_options[i].c_str();
                        if(ImGui::Selectable(connection_options[i].c_str(), selected)) current = connection_options[i].c_str();
                        if(selected) ImGui::SetItemDefaultFocus();
                    }

                    ImGui::EndCombo();
                }

                ImGui::Text("Organization Name: ");
                ImGui::SameLine();
                static char org_name[64];
                ImGui::InputText("##org_name", org_name, IM_ARRAYSIZE(org_name));
                
                ImGui::Text("Workspace Name: ");
                ImGui::SameLine();
                static char workspace_name[64];
                ImGui::InputText("##org_name", workspace_name, IM_ARRAYSIZE(workspace_name));

                ImGui::Text("Description: ");
                static char description[512];
                ImGui::InputTextMultiline("##descri", description, IM_ARRAYSIZE(description), ImVec2(450, 75));

                ImGui::SetCursorPosX((ImGui::GetWindowSize().x - 75) / 2);
                bool input = ImGui::Button("Create", ImVec2(75, 25));


                ImGui::EndTabItem();
            }

            if(ImGui::BeginTabItem("Add Existing")) {
                ImGui::Text("Connection: ");
                ImGui::SameLine();
                if(ImGui::BeginCombo("##connection", current)) {
                    for(int i = 0; i < connections->length; i++) {
                        bool selected = current == connection_options[i].c_str();
                        if(ImGui::Selectable(connection_options[i].c_str(), selected)) current = connection_options[i].c_str();
                        if(selected) ImGui::SetItemDefaultFocus();
                    }

                    ImGui::EndCombo();
                }

                ImGui::Text("Workspace Name: ");
                ImGui::SameLine();
                static char schema[128] = "";
                ImGui::InputText("##workspace_name", schema, IM_ARRAYSIZE(schema));
                ImGui::EndTabItem();

                ImGui::SetCursorPosX((ImGui::GetWindowSize().x - 75) / 2);
                bool input = ImGui::Button("Add", ImVec2(75, 25));

                //TODO - SQL stuff to verify existing workspace


            }

            if (ImGui::BeginTabItem("Add Connection")) {
                ImGui::Text("Database IP: ");
                ImGui::SameLine();
                static char ip[128] = "";
                ImGui::PushItemWidth(250);
                ImGui::SetCursorPosX(150);
                ImGui::InputText("##ip", ip, IM_ARRAYSIZE(ip));

                ImGui::Text("Port:");
                ImGui::SameLine();
                static int port = 3306;
                ImGui::SetCursorPosX(150);
                ImGui::InputInt("##port", &port, 0, 0);
                if (port < 0)
                    port = 0;
                else if (port > 65535)
                    port = 65535;

                ImGui::Text("Username: ");
                ImGui::SameLine();
                static char username[128];
                ImGui::SetCursorPosX(150);
                ImGui::InputText("##username", username, IM_ARRAYSIZE(username));

                static bool show_pass = false;

                ImGui::Text("Password: ");
                ImGui::SameLine();
                static char password[128];
                ImGui::SetCursorPosX(150);
                ImGui::InputText("##password", password, IM_ARRAYSIZE(password), (show_pass ? 0 : ImGuiInputTextFlags_Password) | ImGuiInputTextFlags_EnterReturnsTrue);

                ImGui::SetCursorPosX(150);
                ImGui::Checkbox("Show", &show_pass);

                ImGui::NewLine();
                ImGui::SetCursorPosX(200);
                bool connect_ready = ImGui::Button("Connect", ImVec2(100, 25));
                static bool show_login_error = false;

                if (connect_ready && !(ip[0] == '\0' || username[0] == '\0' || password[0] == '\0')) {
                    sql::Connection* con;
                    try {
                        connect_ready = false;
                        sql::Connection* con = get_driver_instance()->connect("tcp://" + ((std::string)ip) + ":" + std::to_string(port), (std::string)username, (std::string)password);
                        tasker::add_json_connection(tasker::json_sql_connection{(std::string)ip, port, (std::string)username, (std::string)password, new std::string{""}});
                        show_login_error = false;
                        delete con;
                    } catch (sql::SQLException& e) {
                        // delete con;
                        show_login_error = true;
                    }
                }

                if (show_login_error) {
                    ImGui::GetStyle().Colors[ImGuiCol_Text] = tasker::Colors::error_text;
                    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Could not connect! Check login info!").x) / 2);
                    ImGui::Text("Could not connect! Check login info!");
                    ImGui::GetStyle().Colors[ImGuiCol_Text] = tasker::Colors::text;
                }

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::End();
    }

}

void display_worskapce_selection(tasker::json_sql_connection& connection, tasker::DisplayWindowStage& stage, int& latestId) {
    static bool refresh = true;
    static tasker::json_sql_connection_array* connections;
    if (refresh) {
        connections = tasker::get_json_connections();
        refresh = false;
    }

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse;
    ImGui::SetNextWindowSize(ImGui::GetMainViewport()->Size);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->Pos);
    if (ImGui::Begin("Select Workspace", nullptr, flags)) {
        ImDrawList* draw = ImGui::GetWindowDrawList();

        constexpr int menu_offset = 27;
        int spacing = 40;
        float rounding = 15;
        ImVec2 size(200, 150);
        int posY = 25 + menu_offset;

        int row_size;
        int side_padding = 20;
        for (row_size = 1; (row_size * size.x) + (spacing * (row_size - 1)) + (2 * side_padding) <= ImGui::GetWindowSize().x; row_size++)
            ;
        if (row_size > 1) row_size--;
        side_padding = (ImGui::GetWindowSize().x - ((row_size * size.x) + (spacing * (row_size - 1)))) / 2;
        int originalPosX = side_padding;
        int posX = originalPosX;

        int current_box = 1;
        bool picked;

        for (int i = 0; i < connections->length; i++) {
            for (int j = 0; j < (connections->connections + i)->schema_number; j++) {
                if (*((connections->connections + i)->schema + j) == "") continue;
                ImGui::PushID(latestId++);
                ImGui::SetCursorPosX(posX);
                ImGui::SetCursorPosY(posY);
                picked = ImGui::InvisibleButton("", size);
                ImGui::PopID();
                bool selected = ImGui::IsItemHovered();
                draw->AddRectFilled(ImVec2(posX, posY), ImVec2(posX + size.x, posY + size.y),
                                    ImColor((selected ? tasker::Colors::active : tasker::Colors::background)), rounding);

                ImVec2 text_size = ImGui::CalcTextSize(((connections->connections + i)->schema + j)->c_str());
                ImGui::SetCursorPosX(posX + (size.x - text_size.x) / 2);
                ImGui::SetCursorPosY(posY + (size.y - text_size.y) / 2);
                ImGui::TextUnformatted(((connections->connections + i)->schema + j)->c_str());
                ImGui::NewLine();
                std::string ip = (((connections->connections + i)->ip) + ":" + std::to_string((connections->connections + i)->port)).c_str();
                ImGui::SetCursorPosX(posX + (size.x - ImGui::CalcTextSize(ip.c_str()).x) / 2);
                ImGui::TextUnformatted(ip.c_str());

                posX += spacing + size.x;
                if (current_box % row_size == 0) {
                    posY += spacing + size.y;
                    posX = originalPosX;
                }
                current_box++;

                if (picked) {
                    connection = *(connections->connections + i);
                    stage = tasker::DisplayWindowStage::workspace_main;
                }
            }
        }
        static bool add_connection;

        ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x / 2) - 65, posY + spacing + size.y));
        if (ImGui::Button("Add", ImVec2(50, 50)) || add_connection) {
            add_connection = true;
            display_connection_add(add_connection);
        }
        // TODO fix positioning
        ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x / 2) - 5, posY + spacing + size.y));
        refresh = ImGui::Button("Refresh", ImVec2(75, 50));

        ImGui::End();
    }
}