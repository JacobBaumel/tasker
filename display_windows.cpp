#include "display_windows.h"
#include <cppconn/connection.h>
#include <cppconn/driver.h>
#include "Colors.h"
#include "imgui.h"
#include "db_functions.h"
#include "jsonstuff.h"

void display_connection_add(bool& add_connection, tasker::database_array* connections, bool& refresh) {
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;
    ImGui::SetNextWindowSize(ImVec2(500, 300));
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + (viewport->Size.x / 2) - 250, viewport->Pos.y + (viewport->Size.y / 2) - 250));
    
    static std::vector<std::string>* connection_options = NULL;
    static const char* current = NULL;
    bool combo_focus = false;

    if (ImGui::Begin("Add new workspace", &add_connection, flags)) {
        if (ImGui::BeginTabBar("AddTab")) {
            if(refresh || connection_options == NULL) {
                delete connection_options;
                tasker::get_databases(connections);
                connection_options = new std::vector<std::string>();
                current = NULL;

                for(int i = 0; i < connections->connections.size(); i++) {
                    std::string option = connections->get_connection(i).ip;
                    option.append(":").append(std::to_string(connections->get_connection(i).port));
                    if(i == 0) connection_options->push_back(option);
                    else if(option != connection_options->at(connection_options->size() - 1)) {
                        connection_options->push_back(option);
                    }
                }

                if(connection_options->size() > 0) current = connection_options->at(0).c_str();   
            }

            if(connection_options->size() > 0) {
                if(ImGui::BeginTabItem("Create New")) {
                ImGui::Text("Connection: ");
                ImGui::SameLine();
                
                if(ImGui::BeginCombo("##connectionnn", current)) {
                    combo_focus = true;
                    for(int i = 0; i < connection_options->size(); i++) {
                        bool selected = current == connection_options->at(i).c_str();
                        if(ImGui::Selectable(connection_options->at(i).c_str(), selected)) current = connection_options->at(i).c_str();
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
                ImGui::InputText("##work_name", workspace_name, IM_ARRAYSIZE(workspace_name));

                ImGui::Text("Description: ");
                static char description[512];
                ImGui::InputTextMultiline("##descri", description, IM_ARRAYSIZE(description), ImVec2(450, 75));

                ImGui::SetCursorPosX((ImGui::GetWindowSize().x - 75) / 2);

                if(ImGui::Button("Create", ImVec2(75, 25))) {
                    refresh = true;
                }


                ImGui::EndTabItem();
            }

            if(ImGui::BeginTabItem("Add Existing")) {
                ImGui::Text("Connection: ");
                ImGui::SameLine();
                if(ImGui::BeginCombo("##connection", current)) {
                    combo_focus = true;
                    for(int i = 0; i < connection_options->size(); i++) {
                        bool selected = current == connection_options->at(i).c_str();
                        if(ImGui::Selectable(connection_options->at(i).c_str(), selected)) current = connection_options->at(i).c_str();
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

                static tasker::return_code does_exist = tasker::None;
                int conn_number;
                if(input) {
                    for(conn_number = 0; conn_number < connections->connections.size(); conn_number++) if(current == connection_options->at(conn_number)) break;
                    if(tasker::set_connection(connections->get_connection(conn_number)) != tasker::Error) 
                        does_exist = tasker::does_workspace_exist(std::string(schema));
                    else does_exist = tasker::Error;
                }

                if(does_exist == tasker::False) {
                    ImGui::GetStyle().Colors[ImGuiCol_Text] = tasker::Colors::error_text;
                    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Workspace does not exist! Please create it!").x) / 2);
                    ImGui::Text("Workspace does not exist! Please create it!");
                    ImGui::GetStyle().Colors[ImGuiCol_Text] = tasker::Colors::text;
                }

                else if(does_exist == tasker::Error) {
                    ImGui::GetStyle().Colors[ImGuiCol_Text] = tasker::Colors::error_text;
                    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Error connecting to database! D:").x) / 2);
                    ImGui::Text("Error connecting to database! D:");
                    ImGui::GetStyle().Colors[ImGuiCol_Text] = tasker::Colors::text;
                }

                else if(does_exist == tasker::True) {
                    ImGui::GetStyle().Colors[ImGuiCol_Text] = tasker::Colors::green;
                    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Sucess!").x) / 2);
                    ImGui::Text("Sucess!");
                    ImGui::GetStyle().Colors[ImGuiCol_Text] = tasker::Colors::text;

                    if(input) {
                        tasker::add_json_database(tasker::json_database{connections->get_connection(conn_number), std::string(schema)});
                        refresh = true;
                        add_connection = false;
                    }
                }


            }
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
                static tasker::return_code success = tasker::return_code::False;

                if (ImGui::Button("Connect", ImVec2(100, 25)) && !(ip[0] == '\0' || username[0] == '\0' || password[0] == '\0')) {
                    sql::Connection* con;
                    try {
                        con = get_driver_instance()->connect("tcp://" + ((std::string)ip) + ":" + std::to_string(port), (std::string)username, (std::string)password);
                        tasker::add_json_connection(tasker::json_sql_connection{(std::string)ip, port, (std::string)username, (std::string)password});
                        delete con;
                        refresh = true;
                        success = tasker::return_code::True;
                    } catch (sql::SQLException& e) {
                        success = tasker::return_code::Error;
                    }
                }

                if (success == tasker::return_code::Error) {
                    ImGui::GetStyle().Colors[ImGuiCol_Text] = tasker::Colors::error_text;
                    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Could not connect! Check login info!").x) / 2);
                    ImGui::Text("Could not connect! Check login info!");
                    ImGui::GetStyle().Colors[ImGuiCol_Text] = tasker::Colors::text;
                }

                if(success == tasker::return_code::True) {
                    ImGui::GetStyle().Colors[ImGuiCol_Text] = tasker::Colors::green;
                    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Success!").x) / 2);
                    ImGui::Text("Success!");
                    ImGui::GetStyle().Colors[ImGuiCol_Text] = tasker::Colors::text;
                    refresh = true;
                }

                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }
        if(!add_connection) add_connection = ImGui::IsWindowFocused() || combo_focus;
        ImGui::End();
    }

    if(!add_connection) {
        delete connection_options;
        connection_options = NULL;
    }

}

void display_worskapce_selection(tasker::json_database& connection, tasker::DisplayWindowStage& stage, int& latestId, bool& refresh, tasker::static_pointers& pointers) {
    
    tasker::database_array* connections = (tasker::database_array*) pointers.p1;
    if (refresh) {
        delete connections;
        pointers.p1 = new tasker::database_array{};
        connections = (tasker::database_array*) pointers.p1;
        tasker::get_databases(connections);
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
        for (row_size = 1; (row_size * size.x) + (spacing * (row_size - 1)) + (2 * side_padding) <= ImGui::GetWindowSize().x; row_size++);
        if (row_size > 1) row_size--;
        side_padding = (ImGui::GetWindowSize().x - ((row_size * size.x) + (spacing * (row_size - 1)))) / 2;
        int originalPosX = side_padding;
        int posX = originalPosX;

        int current_box = 1;
        bool picked;
        for (int i = 0; i < connections->databases.size(); i++) {
            if (connections->get_database(i).schema == "") continue;
            ImGui::PushID(latestId++);
            ImGui::SetCursorPosX(posX);
            ImGui::SetCursorPosY(posY);
            picked = ImGui::InvisibleButton("##picked", size);
            ImGui::PopID();
            bool selected = ImGui::IsItemHovered();
            draw->AddRectFilled(ImVec2(posX, posY), ImVec2(posX + size.x, posY + size.y),
                                ImColor((selected ? tasker::Colors::active : tasker::Colors::background)), rounding);

            ImVec2 text_size = ImGui::CalcTextSize(connections->get_database(i).schema.c_str());
            ImGui::SetCursorPosX(posX + (size.x - text_size.x) / 2);
            ImGui::SetCursorPosY(posY + (size.y - text_size.y) / 2);
            ImGui::TextUnformatted(connections->get_database(i).schema.c_str());
            ImGui::NewLine();
            std::string ip = connections->get_database(i).connection.ip + ":" + std::to_string(connections->get_database(i).connection.port);
            ImGui::SetCursorPosX(posX + (size.x - ImGui::CalcTextSize(ip.c_str()).x) / 2);
            ImGui::TextUnformatted(ip.c_str());
            posX += spacing + size.x;
            if (current_box % row_size == 0) {
                posY += spacing + size.y;
                posX = originalPosX;
            }

            current_box++;
            if (picked) {
                connection = connections->get_database(i);
                stage = tasker::DisplayWindowStage::workspace_main;
            }
        }
        static bool add_connection;

        ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x / 2) - 65, posY + spacing + size.y));
        if (ImGui::Button("Add", ImVec2(50, 50)) || add_connection) {
            add_connection = true;
            display_connection_add(add_connection, connections, refresh);
        }
        // TODO fix positioning
        ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x / 2) - 5, posY + spacing + size.y));
        refresh = ImGui::Button("Refresh", ImVec2(75, 50)) || refresh;

        ImGui::End();

    }
}