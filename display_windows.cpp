#define TASKER_DEBUG

#include "display_windows.h"
#include <cppconn/connection.h>
#include <cppconn/driver.h>
#include "Colors.h"
#include "imgui.h"
#include "db_functions.h"
#include "jsonstuff.h"
#include "ctime"


#ifdef TASKER_DEBUG
void print_workspace(const tasker::workspace& config) {
    std::cout << "Workspace name: " << config.name << std::endl << "Available stati: " << std::endl;
    for(tasker::status* s : config.stati) std::cout << s->name << ": " << s->color << std::endl;
    std::cout << std::endl;
    for(tasker::supertask* s : config.tasks) {
        std::cout << s->name << ": " << s->color << std::endl << "Tasks:" << std::endl;
        for(tasker::task* t : s->tasks) {
            std::cout << t->taskk << std::endl; 
            std::cout << "Status: " << (t->statuss->name) << std::endl << "Date: " << t->date << std::endl << "People: " << t->people << std::endl;
        }
        std::cout << std::endl;
    }
}
#endif

ImVec4 alphaShift(ImVec4 in, float alpha) {
    return ImVec4(in.x, in.y, in.z, alpha);
}

void display_connection_add(bool& add_connection, tasker::database_array& connections, bool& refresh, tasker::connection_add_statics& statics) {
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;
    ImGui::SetNextWindowSize(ImVec2(500, 300));
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + (viewport->Size.x / 2) - 250, viewport->Pos.y + (viewport->Size.y / 2) - 250));
    
    bool combo_focus = false;

    if (ImGui::Begin("Add new workspace", &add_connection, flags)) {
        if (ImGui::BeginTabBar("AddTab")) {
            if(refresh || statics.connection_options == NULL) {
                delete statics.connection_options;
                tasker::get_databases(connections);
                statics.connection_options = new std::vector<std::string>();
                statics.current = NULL;

                for(int i = 0; i < connections.connections.size(); i++) {
                    std::string option = connections.get_connection(i).ip;
                    option.append(":").append(std::to_string(connections.get_connection(i).port));
                    if(i == 0) statics.connection_options->push_back(option);
                    else if(option != statics.connection_options->at(statics.connection_options->size() - 1)) {
                        statics.connection_options->push_back(option);
                    }
                }

                if(statics.connection_options->size() > 0) statics.current = statics.connection_options->at(0).c_str();   
            }

            if(statics.connection_options->size() > 0) {
                if(ImGui::BeginTabItem("Create New")) {
                ImGui::Text("Connection: ");
                ImGui::SameLine();
                
                if(ImGui::BeginCombo("##connectionnn", statics.current)) {
                    combo_focus = true;
                    for(int i = 0; i < statics.connection_options->size(); i++) {
                        bool selected = statics.current == statics.connection_options->at(i).c_str();
                        if(ImGui::Selectable(statics.connection_options->at(i).c_str(), selected)) statics.current = statics.connection_options->at(i).c_str();
                        if(selected) ImGui::SetItemDefaultFocus();
                    }

                    ImGui::EndCombo();
                }
                
                ImGui::Text("Workspace Name: ");
                ImGui::SameLine();
                ImGui::InputText("##work_name", statics.workspace_name, 64);

                ImGui::SetCursorPosX((ImGui::GetWindowSize().x - 75) / 2);

                int conn_number;
                if(ImGui::Button("Create", ImVec2(75, 25))) {
                    for(conn_number = 0; conn_number < connections.connections.size(); conn_number++) if(statics.current == statics.connection_options->at(conn_number)) break;
                    if(tasker::set_connection(connections.get_connection(conn_number)) == tasker::return_code::Error) statics.success1 = tasker::return_code::Error;
                    else if((statics.success1 = tasker::does_workspace_exist(std::string(statics.workspace_name))) == tasker::return_code::False) {
                        statics.success1 = tasker::create_workspace(statics.workspace_name);
                    }
                    tasker::add_json_database(tasker::json_database{connections.get_connection(conn_number), std::string(statics.workspace_name)});
                    refresh = true;
                }

                if(statics.success1 == tasker::return_code::True) {
                    ImGui::GetStyle().Colors[ImGuiCol_Text] = tasker::Colors::green;
                    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Sucess!").x) / 2);
                    ImGui::Text("Sucess!");
                    ImGui::GetStyle().Colors[ImGuiCol_Text] = tasker::Colors::text;
                }

                else if(statics.success1 == tasker::return_code::False) {
                    ImGui::GetStyle().Colors[ImGuiCol_Text] = ImVec4(1, 0.65, 0, 1);
                    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Workspace already exists! Adding now.").x) / 2);
                    ImGui::Text("Workspace already exists!");
                    ImGui::GetStyle().Colors[ImGuiCol_Text] = tasker::Colors::text;
                }

                else if(statics.success1 == tasker::return_code::Error) {
                    ImGui::GetStyle().Colors[ImGuiCol_Text] = tasker::Colors::error_text;
                    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Error connecting to database! D:").x) / 2);
                    ImGui::Text("Error connecting to database! D:");
                    ImGui::GetStyle().Colors[ImGuiCol_Text] = tasker::Colors::text;
                }


                ImGui::EndTabItem();
            }

            if(ImGui::BeginTabItem("Add Existing")) {
                ImGui::Text("Connection: ");
                ImGui::SameLine();
                if(ImGui::BeginCombo("##connection", statics.current)) {
                    combo_focus = true;
                    for(int i = 0; i < statics.connection_options->size(); i++) {
                        bool selected = statics.current == statics.connection_options->at(i).c_str();
                        if(ImGui::Selectable(statics.connection_options->at(i).c_str(), selected)) statics.current = statics.connection_options->at(i).c_str();
                        if(selected) ImGui::SetItemDefaultFocus();
                    }

                    ImGui::EndCombo();
                }

                ImGui::Text("Workspace Name: ");
                ImGui::SameLine();
                ImGui::InputText("##workspace_name", statics.schema, IM_ARRAYSIZE(statics.schema));
                ImGui::EndTabItem();

                ImGui::SetCursorPosX((ImGui::GetWindowSize().x - 75) / 2);
                bool input = ImGui::Button("Add", ImVec2(75, 25));

                int conn_number;
                if(input) {
                    for(conn_number = 0; conn_number < connections.connections.size(); conn_number++) if(statics.current == statics.connection_options->at(conn_number)) break;
                    if(tasker::set_connection(connections.get_connection(conn_number)) != tasker::return_code::Error) 
                        statics.success2 = tasker::does_workspace_exist(std::string(statics.schema));
                    else statics.success2 = tasker::return_code::Error;
                }

                if(statics.success2 == tasker::return_code::False) {
                    ImGui::GetStyle().Colors[ImGuiCol_Text] = tasker::Colors::error_text;
                    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Workspace does not exist! Please create it!").x) / 2);
                    ImGui::Text("Workspace does not exist! Please create it!");
                    ImGui::GetStyle().Colors[ImGuiCol_Text] = tasker::Colors::text;
                }

                else if(statics.success2 == tasker::return_code::Error) {
                    ImGui::GetStyle().Colors[ImGuiCol_Text] = tasker::Colors::error_text;
                    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Error connecting to database! D:").x) / 2);
                    ImGui::Text("Error connecting to database! D:");
                    ImGui::GetStyle().Colors[ImGuiCol_Text] = tasker::Colors::text;
                }

                else if(statics.success2 == tasker::return_code::True) {
                    ImGui::GetStyle().Colors[ImGuiCol_Text] = tasker::Colors::green;
                    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Sucess!").x) / 2);
                    ImGui::Text("Sucess!");
                    ImGui::GetStyle().Colors[ImGuiCol_Text] = tasker::Colors::text;

                    if(input) {
                        tasker::add_json_database(tasker::json_database{connections.get_connection(conn_number), std::string(statics.schema)});
                        refresh = true;
                        add_connection = false;
                    }
                }


            }
            }

            if (ImGui::BeginTabItem("Add Connection")) {
                ImGui::Text("Database IP: ");
                ImGui::SameLine();
                ImGui::PushItemWidth(250);
                ImGui::SetCursorPosX(150);
                ImGui::InputText("##ip", statics.ip, IM_ARRAYSIZE(statics.ip));

                ImGui::Text("Port:");
                ImGui::SameLine();
                ImGui::SetCursorPosX(150);
                ImGui::InputInt("##port", &statics.port, 0, 0);
                if (statics.port < 0)
                    statics.port = 0;
                else if (statics.port > 65535)
                    statics.port = 65535;

                ImGui::Text("Username: ");
                ImGui::SameLine();
                ImGui::SetCursorPosX(150);
                ImGui::InputText("##username", statics.username, IM_ARRAYSIZE(statics.username));

                ImGui::Text("Password: ");
                ImGui::SameLine();
                ImGui::SetCursorPosX(150);
                ImGui::InputText("##password", statics.password, IM_ARRAYSIZE(statics.password), (statics.show_pass ? 0 : ImGuiInputTextFlags_Password) | ImGuiInputTextFlags_EnterReturnsTrue);

                ImGui::SetCursorPosX(150);
                ImGui::Checkbox("Show", &statics.show_pass);

                ImGui::NewLine();
                ImGui::SetCursorPosX(200);
                
                if (ImGui::Button("Connect", ImVec2(100, 25)) && !(statics.ip[0] == '\0' || statics.username[0] == '\0' || statics.password[0] == '\0')) {
                    sql::Connection* con;
                    try {
                        con = get_driver_instance()->connect("tcp://" + ((std::string)statics.ip) + ":" + std::to_string(statics.port), (std::string)statics.username, (std::string)statics.password);
                        tasker::add_json_connection(tasker::json_sql_connection{(std::string)statics.ip, statics.port, (std::string)statics.username, (std::string)statics.password});
                        delete con;
                        refresh = true;
                        statics.success3 = tasker::return_code::True;
                    } catch (sql::SQLException& e) {
                        statics.success3 = tasker::return_code::Error;
                    }
                }

                if (statics.success3 == tasker::return_code::Error) {
                    ImGui::GetStyle().Colors[ImGuiCol_Text] = tasker::Colors::error_text;
                    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Could not connect! Check login info!").x) / 2);
                    ImGui::Text("Could not connect! Check login info!");
                    ImGui::GetStyle().Colors[ImGuiCol_Text] = tasker::Colors::text;
                }

                if(statics.success3 == tasker::return_code::True) {
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
        if(add_connection) add_connection = ImGui::IsWindowFocused() || combo_focus;
        ImGui::End();
    }

    if(!add_connection) {
        delete statics.connection_options;
        statics.connection_options = NULL;
    }

}

void display_worskapce_selection(tasker::json_database& connection, tasker::DisplayWindowStage& stage, int& latestId, bool& refresh, tasker::database_array& connections, bool& add_connection, tasker::connection_add_statics& statics) {
    
    if (refresh) {
        connections = tasker::database_array();
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
        for (int i = 0; i < connections.databases.size(); i++) {
            if (connections.get_database(i).schema == "") continue;
            float scroll = ImGui::GetScrollY();
            ImGui::PushID(latestId++);
            ImGui::SetCursorPos(ImVec2(posX, posY));
            picked = ImGui::InvisibleButton("##picked" + latestId, ImVec2(size.x - 25, size.y));
            ImGui::PopID();
            bool selected = ImGui::IsItemHovered();

            ImGui::PushID(latestId++);
            ImGui::SetCursorPos(ImVec2(posX + size.x - 25, posY + 20));
            picked = picked || ImGui::InvisibleButton("##picked" + latestId, ImVec2(25, size.y - 20));
            ImGui::PopID();
            selected = selected || ImGui::IsItemHovered();

            draw->AddRectFilled(ImVec2(posX, posY - scroll), ImVec2(posX + size.x, posY + size.y - scroll),
                                ImColor((selected ? tasker::Colors::active : tasker::Colors::background)), rounding);

            ImVec2 text_size = ImGui::CalcTextSize(connections.get_database(i).schema.c_str());
            ImGui::SetCursorPosX(posX + (size.x - text_size.x) / 2);
            ImGui::SetCursorPosY(posY + (size.y - text_size.y) / 2);
            ImGui::TextUnformatted(connections.get_database(i).schema.c_str());
            ImGui::NewLine();
            std::string ip = connections.get_database(i).connection.ip + ":" + std::to_string(connections.get_database(i).connection.port);
            ImGui::SetCursorPosX(posX + (size.x - ImGui::CalcTextSize(ip.c_str()).x) / 2);
            ImGui::TextUnformatted(ip.c_str());

            ImGui::PushID(latestId++);
            ImGui::SetCursorPos(ImVec2(posX + size.x - 25, posY + 5));
            bool pushed = ImGui::InvisibleButton("##delete", ImVec2(15, 15));
            bool hovered = ImGui::IsItemHovered();
            ImGui::PopID();
            
            draw->AddLine(ImVec2(posX + size.x - 25, posY + 5 - scroll), ImVec2(posX + size.x - 10, posY + 20 - scroll), (hovered ? ImGui::GetColorU32(ImVec4(1, 1, 1, 1)) : ImGui::GetColorU32(tasker::Colors::green)), 1.5);
            draw->AddLine(ImVec2(posX + size.x - 25, posY + 20 - scroll), ImVec2(posX + size.x - 10, posY + 5 - scroll), (hovered ? ImGui::GetColorU32(ImVec4(1, 1, 1, 1)) : ImGui::GetColorU32(tasker::Colors::green)), 1.5);
            
            posX += spacing + size.x;
            if (current_box % row_size == 0) {
                posY += spacing + size.y;
                posX = originalPosX;
            }

            current_box++;
            if (picked) {
                connection = connections.get_database(i);
                stage = tasker::DisplayWindowStage::workspace_main;
            }
            
            if(pushed) {
                tasker::remove_json_database(connections.get_database(i));
                refresh = true;
            }
        }

        ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x / 2) - 65, posY + spacing + size.y));
        if (ImGui::Button("Add", ImVec2(50, 50)) || add_connection) {
            add_connection = true;
            display_connection_add(add_connection, connections, refresh, statics);
        }
        // TODO fix positioning
        ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x / 2) - 5, posY + spacing + size.y));
        refresh = ImGui::Button("Refresh", ImVec2(75, 50)) || refresh;

        ImGui::End();

    }
}

void draw_supertask(tasker::supertask* task, int& y, int& latestId, std::vector<tasker::status*>& stati, const char* schema, bool& refresh);
void create_new(bool& create_cat, float* colors, char* buffer, const int& buff_size, bool& refresh);
void manage_statuses(bool& manage_statuses, bool& refresh, std::vector<tasker::status*>& stati, int& latestId, tasker::workspace& workspace, float* colors);

void display_workspace(tasker::json_database& database, tasker::DisplayWindowStage& stage, int& latestId, bool& refresh, tasker::workspace& config, time_t& timer) {
    if(refresh) {
        tasker::set_connection(database.connection);
        tasker::set_schema(database.schema);
        if(tasker::get_data(config) == tasker::return_code::Error) std::cout << "Error getting data from workspace!!" << std::endl;
        //print_workspace(config);
    }
    
    
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse;
    ImGui::SetNextWindowSize(ImGui::GetMainViewport()->Size);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->Pos);
    bool open = true;
    if (ImGui::Begin(database.schema.c_str(), &open, flags)) {
        int y = 100 - ImGui::GetScrollY();

        ImGui::SetCursorPos(ImVec2(50, 50));
        if(ImGui::Button("New Category", ImVec2(150, 25)) || config.create_cat) {
            if(!config.create_cat) std::fill(config.new_category, config.new_category + 255, '\0');
            config.create_cat = true;
            create_new(config.create_cat, config.new_color, config.new_category, IM_ARRAYSIZE(config.new_category), refresh);
        }
        ImGui::SameLine();
        if(ImGui::Button("Manage Statuses", ImVec2(150, 25)) || config.manage_statuses) {
            if(!config.manage_statuses) std::fill(config.new_category, config.new_category + 255, '\0');
            config.manage_statuses = true;
            manage_statuses(config.manage_statuses, refresh, config.stati, latestId, config, config.new_color);
        }
        ImGui::SameLine();
        if(ImGui::Button("Refresh", ImVec2(100, 25))) {
            refresh = true;
            time(&timer);
        }

        {
            time_t temp;
            time(&temp);
            if(abs(timer - temp) < 6) {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0, 1, 0, 1), "Refreshed!");
            }
        }
        for(tasker::supertask* s : config.tasks) draw_supertask(s, y, latestId, config.stati, &database.schema[0], refresh);
        ImGui::Dummy(ImVec2(0, y + 100));
    }
    ImGui::End();

    if(!open) {
        stage = tasker::DisplayWindowStage::pick_workspace;
        refresh = true;
    }
}

void draw_supertask(tasker::supertask* task, int& y, int& latestId, std::vector<tasker::status*>& stati, const char* schema, bool& refresh) {
    float scroll = ImGui::GetScrollY();
    ImDrawList* draw = ImGui::GetWindowDrawList();
    bool isDark = ((task->color.Value.x + task->color.Value.y + task->color.Value.z) / 3) < 0.33;

    ImColor main_color = isDark ? ImColor(ImVec4(1, 1, 1, 1)) : ImColor(ImVec4(0, 0, 0, 1));
    ImColor accent = isDark ? ImColor(ImVec4(0, 0, 0, 1)) : ImColor(ImVec4(1, 1, 1, 1));
    ImGui::PushStyleColor(ImGuiCol_FrameBg, alphaShift(task->color.Value, 0.25));
    ImGui::PushStyleColor(ImGuiCol_Text, main_color.Value);
    if(isDark) draw->AddRectFilled(ImVec2(48, y - 2), ImVec2(ImGui::GetWindowSize().x - 48, y + 37), ImColor(ImVec4(1, 1, 1, 1)), 5);
    draw->AddRectFilled(ImVec2(50, y - ImGui::GetScrollY()), ImVec2(ImGui::GetWindowSize().x - 50, y + 35 - ImGui::GetScrollY()), task->color, 5);
    ImGui::SetCursorPos(ImVec2(75, y + 7));
    ImGui::TextUnformatted(task->display_name);
    ImGui::PopStyleColor();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));

    ImGui::PushID(latestId++);
    ImGui::SetCursorPos(ImVec2(ImGui::GetWindowSize().x - 75, y + 10 - scroll));
    bool pushedd = ImGui::InvisibleButton("##delete", ImVec2(15, 15));
    bool hovered = ImGui::IsItemHovered();
    ImGui::PopID();

    draw->AddLine(ImVec2(ImGui::GetWindowSize().x - 75, y + 10 - scroll), ImVec2(ImGui::GetWindowSize().x - 60, y + 25 - scroll), (hovered ? ImGui::GetColorU32(accent.Value) : ImGui::GetColorU32(main_color.Value)), 1.5);
    draw->AddLine(ImVec2(ImGui::GetWindowSize().x - 75, y + 25 - scroll), ImVec2(ImGui::GetWindowSize().x - 60, y + 10 - scroll), (hovered ? ImGui::GetColorU32(accent.Value) : ImGui::GetColorU32(main_color.Value)), 1.5);

    if(pushedd) {
        tasker::drop_category(task->name);
        refresh = true;
    }

    if(!task->collapsed) draw->AddTriangleFilled(ImVec2(55, y + 15 - scroll), ImVec2(62.5, y + 25 - scroll), ImVec2(70, y + 15 - scroll), main_color);
    else draw->AddTriangleFilled(ImVec2(60, y + 10 - scroll), ImVec2(60, y + 25 - scroll), ImVec2(67.5, y + 17.5 - scroll), main_color);
    
    ImGui::SetCursorPos(ImVec2(55, y + 10));
    bool pushed = ImGui::InvisibleButton(std::string("##task_opening_").append(task->name).c_str(), ImVec2(15, 15));
    if(ImGui::IsItemHovered()) draw->AddCircleFilled(ImVec2(62.5, y + 17.5 - scroll), 9, ImColor(ImVec4(0.5, 0.5, 0.5, 0.5)));
    if(!task->collapsed) {
        y += 40;
        for(tasker::task* t : task->tasks) {
            draw->AddRectFilled(ImVec2(50, y - ImGui::GetScrollY()), ImVec2(ImGui::GetWindowSize().x - 50, y + 35 - ImGui::GetScrollY()), ImColor(ImVec4(0.25, 0.25, 0.25, 1)), 5);
            draw->AddRectFilled(ImVec2(50, y - ImGui::GetScrollY()), ImVec2(55, y + 35 - ImGui::GetScrollY()), task->color);

            bool changed = false;

            ImGui::SetCursorPos(ImVec2(60, y + 3));
            ImGui::PushItemWidth((ImGui::GetWindowSize().x - 100) / 2);
            changed = changed || ImGui::InputText(std::string("##task_input_").append(std::to_string(latestId++)).c_str(), &t->taskk[0], IM_ARRAYSIZE(t->taskk), ImGuiInputTextFlags_EnterReturnsTrue);
            changed = changed || ImGui::IsItemActive();
            ImGui::PopItemWidth();

            ImGui::SetCursorPos(ImVec2((((ImGui::GetWindowSize().x - 100) / 2) + 70), y + 3));
            ImGui::PushItemWidth((ImGui::GetWindowSize().x - 100) / 3);
            bool isTyping = ImGui::InputText(std::string("##task_people_").append(std::to_string(latestId++)).c_str(), &t->people[0], IM_ARRAYSIZE(t->people), ImGuiInputTextFlags_EnterReturnsTrue);
            ImGui::PopItemWidth();
            changed = changed || ImGui::IsItemActive() || isTyping; // Honestly no idea, but separating (just this last one) into a separate bool fixed a weird rendering issue

            ImGui::PushStyleColor(ImGuiCol_FrameBg, t->statuss->color.Value);
            int color_shift = 1.5;
            ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(t->statuss->color.Value.x * color_shift, t->statuss->color.Value.y * color_shift, t->statuss->color.Value.z * color_shift, 0.8));
            ImGui::SetCursorPos(ImVec2((((ImGui::GetWindowSize().x - 100) * (5.0 / 6.0)) + 80), y + 3));
            ImGui::PushItemWidth(((ImGui::GetWindowSize().x - 100) / 6) - 50);
            bool edited = false;
            if(ImGui::BeginCombo(std::string("##status_selector_").append(std::to_string(latestId++)).c_str(), &t->statuss->name[0], ImGuiComboFlags_NoArrowButton)) {
                bool first = true;
                for(int i = 0; i < stati.size(); i++) {
                    bool selected = t->statuss == stati.at(i);
                    ImGui::PushStyleColor(ImGuiCol_Button, alphaShift(stati.at(i)->color.Value, (selected ? 0.9 : 0.8)));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, alphaShift(stati.at(i)->color.Value, 0.95));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, alphaShift(stati.at(i)->color.Value, 1));
                    if(ImGui::Button(&stati.at(i)->name[0], ImVec2(ImGui::GetItemRectMax().x - ImGui::GetItemRectMin().x - (first ? 10 : 0), 0))) {
                        t->statuss = stati.at(i);
                        edited = true;
                    }
                    if(first) first = false;
                    ImGui::PopStyleColor();
                    ImGui::PopStyleColor();
                    ImGui::PopStyleColor();
                    if(selected) ImGui::SetItemDefaultFocus();
                }

                ImGui::EndCombo();
            }
            changed = changed || edited;

            if(edited) ImGui::SetWindowFocus(schema); // This schema is so imgui can focus on the window and close the combo, not for updating mysql

            if(t->wasSelected && !changed) tasker::update_task(task->name, t);
            t->wasSelected = changed;
            ImGui::PopStyleColor();
            ImGui::PopStyleColor();

            
            y += 40;
        }
    }
    if(pushed) task->collapsed = !task->collapsed;

    ImGui::PopStyleColor();
    ImGui::PopStyleColor();
    y += 50;
}

void create_new(bool& create_cat, float* colors, char* buffer, const int& buff_size, bool& refresh) {
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;
    ImGui::SetNextWindowSize(ImVec2(500, 500));
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + (viewport->Size.x / 2) - 250, viewport->Pos.y + (viewport->Size.y / 2) - 250));

    if(ImGui::Begin("Create New Category", &create_cat, flags)) {
        
        ImGui::ColorPicker3("New Color", colors);
        ImGui::TextUnformatted("Category name: ");
        ImGui::SameLine();
        ImGui::InputText("##new_cat_name", buffer, buff_size);

        bool create;
        {
            constexpr int button_size = 100;
            ImGui::SetCursorPosX((ImGui::GetWindowSize().x - button_size) / 2);
            create = ImGui::Button("Create", ImVec2(button_size, 25));
        }
        if(create) {
            tasker::create_category(buffer, colors);
            refresh = true;
            create_cat = false;
        }

        create_cat = create_cat && ImGui::IsWindowFocused();
        ImGui::End();
    }
}

void manage_statuses(bool& manage_statuses, bool& refresh, std::vector<tasker::status*>& stati, int& latestId, tasker::workspace& workspace, float* colors) {
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;
    ImGui::SetNextWindowSize(ImVec2(500, 500));
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + (viewport->Size.x / 2) - 250, viewport->Pos.y + (viewport->Size.y / 2) - 250));

    if(ImGui::Begin("Mange Statuses", &manage_statuses, flags)) {
        for(tasker::status* s : stati) {
            if(std::string(s->name) == "None") continue;
            ImGui::SetCursorPosX(45);
            ImGui::PushStyleColor(ImGuiCol_Button, s->color.Value);
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, s->color.Value);
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, s->color.Value);
            ImGui::Button(std::string(s->name).append("##" + std::to_string(1)).c_str(), ImVec2(200, 50));
            ImGui::PopStyleColor();
            ImGui::PopStyleColor();

            ImGui::SameLine();
            ImGui::SetCursorPosX(255);
            if(ImGui::Button(std::string("Remove##").append(std::to_string(latestId++)).c_str(), ImVec2(200, 50))) {
                tasker::remove_status(s, workspace);
                refresh = true;
                manage_statuses = false;
            }
            ImGui::PopStyleColor();
        }

        ImGui::NewLine();
        ImGui::ColorPicker3("New Color", colors);

        ImGui::TextUnformatted("Status Name: ");
        ImGui::PushID(latestId++);
        ImGui::InputText("##", workspace.new_category, IM_ARRAYSIZE(workspace.new_category));
        ImGui::PopID();

        ImGui::SetCursorPosX(45);
        if(ImGui::Button("Create Status", ImVec2(410, 50))) {
            tasker::create_status(workspace.new_category, colors[0] * 255, colors[1]* 255, colors[2] * 255);
            refresh = true;
        }
        manage_statuses = manage_statuses && ImGui::IsWindowFocused();
        ImGui::End();
    }
}