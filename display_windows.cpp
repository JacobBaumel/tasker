// This file contains all the rendering/drawing code for the whole application
#include "display_windows.h"
#include <cppconn/connection.h>
#include <cppconn/driver.h>
#include <string>
#include "Colors.h"
#include "cppconn/exception.h"
#include "imgui.h"
#include "jsonstuff.h"
#include "ctime"
#include "structs.hpp"
using std::string;

// Helper for slightly adjusting the alpha of an ImVec4 object, so it can be done inline
ImVec4 alphaShift(const ImVec4& in, float alpha) {
    return ImVec4(in.x, in.y, in.z, alpha);
}

constexpr ImVec4 operator*(const ImVec4& im, const float& val) {
    return ImVec4(im.x * val, im.y * val, im.z * val, im.w);
}

long abs(long l) {
    if(l < 0) return l * -1;
    return l;
}

// Function solely for drawing the add screen, called by the master display_workspace_selection function
void display_connection_add(bool& add_connection, tasker::database_array& connections, bool& refresh, tasker::connection_add_statics& statics) {
    // Sets up mini-window
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;
    ImGui::SetNextWindowSize(ImVec2(500, 300));
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + (viewport->Size.x / 2) - 250, viewport->Pos.y + (viewport->Size.y / 2) - 250));

    // Used to check if either the window or combo is selected, to determine if adder needs to close
    bool combo_focus = false;

    // Begin a window + tab bar
    if (ImGui::Begin("Add new workspace", &add_connection, flags)) {
        if (ImGui::BeginTabBar("AddTab")) {
            // Gets new list of connection options for the dropdown from json file if necessary. statics.connection_options is a vector which contains strings of the options
            if(refresh || statics.connection_options == NULL) {
                // If a refresh is initiated, or the options have not been initialized, delete the old option set, get new options from json, and add them to the new list
                delete statics.connection_options;
                tasker::get_databases(connections);
                statics.connection_options = new std::vector<std::string>();
                statics.current = nullptr;

                // Grabs tasker::json_sql_connection objects and appends them as nice looking strings to be displayed in combos
                for(size_t i = 0; i < connections.connections.size(); i++) {
                    std::string option = connections.get_connection(i).ip;
                    option.append(":").append(std::to_string(connections.get_connection(i).port));
                    if(i == 0) statics.connection_options->push_back(option);
                    else if(option != statics.connection_options->at(statics.connection_options->size() - 1)) {
                        statics.connection_options->push_back(option);
                    }
                }

                // Resets the currently selected connection option
                if(statics.connection_options->size() > 0) statics.current = statics.connection_options->at(0).c_str();   
            }

            // Only want to be able to create new workspace or join existing workspace if connection options exist. If not, then only show connection add screen
            if(statics.connection_options->size() > 0) {
                if(ImGui::BeginTabItem("Create New")) {
                    // Selector + label for connection choose
                    ImGui::Text("Connection: ");
                    ImGui::SameLine();
                    if(ImGui::BeginCombo("##connectionnn", statics.current)) {
                        combo_focus = true;
                        for(size_t i = 0; i < statics.connection_options->size(); i++) {
                            bool selected = statics.current == statics.connection_options->at(i).c_str();
                            if(ImGui::Selectable(statics.connection_options->at(i).c_str(), selected)) statics.current = statics.connection_options->at(i).c_str();
                            if(selected) ImGui::SetItemDefaultFocus();
                        }

                        ImGui::EndCombo();
                    }

                    // Text field + label for new workspace
                    ImGui::Text("Workspace Name: ");
                    ImGui::SameLine();
                    ImGui::InputText("##work_name", statics.workspace_name, 64);

                    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - 75) / 2);

                    size_t conn_number;
                    if(ImGui::Button("Create", ImVec2(75, 25))) {
                        // Used to determine index of connection in the list of options is currently selected
                        for(conn_number = 0; conn_number < connections.connections.size(); conn_number++) if(statics.current == statics.connection_options->at(conn_number)) break;

                        try {
                            tasker::workspace w(getConnection(connections.get_connection(conn_number)), statics.workspace_name);
                            w.create();
                            statics.error1 = 0;
                            refresh = true;
                        }

                        catch(sql::SQLException& e) {
                            statics.error1 = 1;
                        }

                        catch(tasker::TaskerException& e) {
                            statics.error1 = *e.code;
                        }

                        if(statics.error1 == 0 || statics.error1 == 3)
                            tasker::add_json_database(tasker::json_database{connections.get_connection(conn_number), std::string(statics.workspace_name)});

                    }

                    // If the workspace already exists and login was successful, display success message
                    if(statics.error1 == 0) {
                        ImGui::GetStyle().Colors[ImGuiCol_Text] = tasker::Colors::green;
                        ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Sucess!").x) / 2);
                        ImGui::Text("Sucess!");
                        ImGui::GetStyle().Colors[ImGuiCol_Text] = tasker::Colors::text;
                    }

                    // If the login was successful, but a workspace had to be created, display appropriate message
                    else if(statics.error1 == 3) {
                        ImGui::GetStyle().Colors[ImGuiCol_Text] = ImVec4(1, 0.65, 0, 1);
                        ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Workspace already exists! Adding now.").x) / 2);
                        ImGui::Text("Workspace already exists!");
                        ImGui::GetStyle().Colors[ImGuiCol_Text] = tasker::Colors::text;
                    }

                    // If login was not successful, display appropriate message
                    else if(statics.error1 == 1) {
                        ImGui::GetStyle().Colors[ImGuiCol_Text] = tasker::Colors::error_text;
                        ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Error connecting to database! D:").x) / 2);
                        ImGui::Text("Error connecting to database! D:");
                        ImGui::GetStyle().Colors[ImGuiCol_Text] = tasker::Colors::text;
                    }


                    ImGui::EndTabItem();
                }

                // Tab for adding already created workspaces, with just a name and connection field
                if(ImGui::BeginTabItem("Add Existing")) {
                    // Connection selector
                    ImGui::Text("Connection: ");
                    ImGui::SameLine();
                    if(ImGui::BeginCombo("##connection", statics.current)) {
                        combo_focus = true;
                        for(size_t i = 0; i < statics.connection_options->size(); i++) {
                            bool selected = statics.current == statics.connection_options->at(i).c_str();
                            if(ImGui::Selectable(statics.connection_options->at(i).c_str(), selected)) statics.current = statics.connection_options->at(i).c_str();
                            if(selected) ImGui::SetItemDefaultFocus();
                        }

                        ImGui::EndCombo();
                    }

                    // Text field for name
                    ImGui::Text("Workspace Name: ");
                    ImGui::SameLine();
                    ImGui::InputText("##workspace_name", statics.schema, IM_ARRAYSIZE(statics.schema));
                    ImGui::EndTabItem();

                    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - 75) / 2);

                    // Similar process to creation: get index of desired connection, set mysql connection + test login, add new workspace to json file
                    if(ImGui::Button("Add", ImVec2(75, 25))) {
                        size_t conn_number;
                        for(conn_number = 0; conn_number < connections.connections.size(); conn_number++) if(statics.current == statics.connection_options->at(conn_number)) break;

                        try {
                            tasker::workspace w(getConnection(connections.get_connection(conn_number)), statics.schema);
                            w.connect(false);
                            tasker::add_json_database(tasker::json_database{connections.get_connection(conn_number), std::string(statics.schema)});
                            refresh = true;
                            add_connection = false;
                        }

                        catch(sql::SQLException& e) {
                            statics.error2 = 1;
                        }

                        catch(tasker::TaskerException& e) {
                            statics.error2 = *e.code;
                        }
                    }

                    // Display appropriate status messages for previous operation
                    if(statics.error2 == 2) {
                        ImGui::GetStyle().Colors[ImGuiCol_Text] = tasker::Colors::error_text;
                        ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Workspace does not exist! Please create it!").x) / 2);
                        ImGui::Text("Workspace does not exist! Please create it!");
                        ImGui::GetStyle().Colors[ImGuiCol_Text] = tasker::Colors::text;
                    }

                    else if(statics.error2 == 1) {
                        ImGui::GetStyle().Colors[ImGuiCol_Text] = tasker::Colors::error_text;
                        ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Error connecting to database! D:").x) / 2);
                        ImGui::Text("Error connecting to database! D:");
                        ImGui::GetStyle().Colors[ImGuiCol_Text] = tasker::Colors::text;
                    }

                    else if(statics.error2 == 0) {
                        ImGui::GetStyle().Colors[ImGuiCol_Text] = tasker::Colors::green;
                        ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Sucess!").x) / 2);
                        ImGui::Text("Sucess!");
                        ImGui::GetStyle().Colors[ImGuiCol_Text] = tasker::Colors::text;
                    }


                }
            }

            // Tab for adding a new possible connection. Should always be visible
            if (ImGui::BeginTabItem("Add Connection")) {
                // Text field for IP address
                ImGui::Text("Database IP: ");
                ImGui::SameLine();
                ImGui::PushItemWidth(250);
                ImGui::SetCursorPosX(150);
                ImGui::InputText("##ip", statics.ip, IM_ARRAYSIZE(statics.ip));

                // Integer field for port number, with the number capped between 0 and 65535
                ImGui::Text("Port:");
                ImGui::SameLine();
                ImGui::SetCursorPosX(150);
                ImGui::InputInt("##port", &statics.port, 0, 0);
                if (statics.port < 0)
                    statics.port = 0;
                else if (statics.port > 65535)
                    statics.port = 65535;

                // Text field for mysql database username
                ImGui::Text("Username: ");
                ImGui::SameLine();
                ImGui::SetCursorPosX(150);
                ImGui::InputText("##username", statics.username, IM_ARRAYSIZE(statics.username));

                // Text field for mysql database password. Will add encryption to serialization eventually
                ImGui::Text("Password: ");
                ImGui::SameLine();
                ImGui::SetCursorPosX(150);
                ImGui::InputText("##password", statics.password, IM_ARRAYSIZE(statics.password), (statics.show_pass ? 0 : ImGuiInputTextFlags_Password) | ImGuiInputTextFlags_EnterReturnsTrue);

                // Checkbox for showing password
                ImGui::SetCursorPosX(150);
                ImGui::Checkbox("Show", &statics.show_pass);

                ImGui::NewLine();
                ImGui::SetCursorPosX(200);
                // Initiates testing the connection details
                if (ImGui::Button("Connect", ImVec2(100, 25)) && !(statics.ip[0] == '\0' || statics.username[0] == '\0' || statics.password[0] == '\0')) {
                    sql::Connection* con;
                    try {
                        // Tries to create a connection to the database, and log in with user/password
                        con = get_driver_instance()->connect("tcp://" + ((std::string)statics.ip) + ":" + std::to_string(statics.port), (std::string)statics.username, (std::string)statics.password);
                        tasker::add_json_connection(tasker::json_sql_connection{(std::string)statics.ip, statics.port, (std::string)statics.username, (std::string)statics.password});
                        delete con;
                        refresh = true;
                        statics.error3 = 0;
                    } catch (sql::SQLException& e) {
                        statics.error3 = 1;
                    }
                }

                // Display appropriate status message
                if (statics.error3 == 1) {
                    ImGui::GetStyle().Colors[ImGuiCol_Text] = tasker::Colors::error_text;
                    ImGui::SetCursorPosX((ImGui::GetWindowSize().x - ImGui::CalcTextSize("Could not connect! Check login info!").x) / 2);
                    ImGui::Text("Could not connect! Check login info!");
                    ImGui::GetStyle().Colors[ImGuiCol_Text] = tasker::Colors::text;
                }

                else if(statics.error3 == 0) {
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

        // If neither the window, connection selector (as it is its own window kinda), or if add_connection goes false, then manually set add_connection to false to close the window 
        if(add_connection) add_connection = ImGui::IsWindowFocused() || combo_focus;
        ImGui::End();
    }

    // Tells beginning of function to refresh connection options
    if(!add_connection) {
        delete statics.connection_options;
        statics.connection_options = NULL;
    }

}

// Function for main selection screen
void display_worskapce_selection(tasker::json_database& connection, tasker::DisplayWindowStage& stage, int& latestId, bool& refresh, tasker::database_array& connections, bool& add_connection, tasker::connection_add_statics& statics) {

    // If a refresh has been started, re-parse connections from json file
    if (refresh) {
        connections = tasker::database_array();
        tasker::get_databases(connections);
    }

    // Sets home window to be full sized, and non-closable
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse;
    ImGui::SetNextWindowSize(ImGui::GetMainViewport()->Size);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->Pos);
    if (ImGui::Begin("Select Workspace", nullptr, flags)) {
        //Drawing object for imgui
        ImDrawList* draw = ImGui::GetWindowDrawList();

        // Some initial values for things like size of selection boxes, spacing between boxes, and rounding of the sides
        constexpr int menu_offset = 27;
        constexpr int spacing = 40;
        constexpr float rounding = 15;
        constexpr ImVec2 size(200, 150);
        const float scroll = ImGui::GetScrollY();

        // Y position to draw selection boxes
        int posY = 25 + menu_offset;

        // Calculates best number of cubes to put in a row, by calculating the x pixel space taken up by row_size boxes, and incrementing row_size until it is larger than the screen
        int row_size;
        int side_padding = 20;
        for (row_size = 1; (row_size * size.x) + (spacing * (row_size - 1)) + (2 * side_padding) <= ImGui::GetWindowSize().x; row_size++);
        if (row_size > 1) row_size--;

        // This re-adjusts the side padding so that different numbers of boxes dont offset the row
        side_padding = (ImGui::GetWindowSize().x - ((row_size * size.x) + (spacing * (row_size - 1)))) / 2;

        // Initial x offset for the boxes
        const int originalPosX = side_padding;
        int posX = originalPosX;

        // Used to determine if a new row should be started
        int current_box = 1;

        bool picked;
        for (size_t i = 0; i < connections.databases.size(); i++) {
            // If a connection is only that, and has no associated worspace, skip drawing a box for it
            if (connections.get_database(i).schema == "") continue;

            // Create an invisible button behind the box so it can be clicked. This section creates a majority of the button on the left of the delete button
            ImGui::PushID(latestId++);
            ImGui::SetCursorPos(ImVec2(posX, posY));
            // Size is offsetted so it does not interfere with the delete button
            picked = ImGui::InvisibleButton(string("##picked").append(std::to_string(latestId)).c_str(), ImVec2(size.x - 25, size.y));
            ImGui::PopID();
            bool selected = ImGui::IsItemHovered();

            // This creates the rest of the button below the delete
            ImGui::PushID(latestId++);
            ImGui::SetCursorPos(ImVec2(posX + size.x - 25, posY + 20));
            picked = picked || ImGui::InvisibleButton(string("##picked").append(std::to_string(latestId)).c_str(), ImVec2(25, size.y - 20));
            ImGui::PopID();
            selected = selected || ImGui::IsItemHovered();

            // Actual drawing of the box
            draw->AddRectFilled(ImVec2(posX, posY - scroll), ImVec2(posX + size.x, posY + size.y - scroll),
                    ImColor((selected ? tasker::Colors::active : tasker::Colors::background)), rounding);

            // Calculates text size so it can be centered in the box
            ImVec2 text_size = ImGui::CalcTextSize(connections.get_database(i).schema.c_str());
            ImGui::SetCursorPosX(posX + (size.x - text_size.x) / 2);
            ImGui::SetCursorPosY(posY + (size.y - text_size.y) / 2);
            ImGui::TextUnformatted(connections.get_database(i).schema.c_str());
            ImGui::NewLine();
            std::string ip = connections.get_database(i).connection.ip + ":" + std::to_string(connections.get_database(i).connection.port);
            ImGui::SetCursorPosX(posX + (size.x - ImGui::CalcTextSize(ip.c_str()).x) / 2);
            ImGui::TextUnformatted(ip.c_str());

            // Creates an inisible button to delete the box
            ImGui::PushID(latestId++);
            ImGui::SetCursorPos(ImVec2(posX + size.x - 25, posY + 5));
            bool pushed = ImGui::InvisibleButton("##delete", ImVec2(15, 15));
            bool hovered = ImGui::IsItemHovered();
            ImGui::PopID();

            // Draws the X for the delete button
            draw->AddLine(ImVec2(posX + size.x - 25, posY + 5 - scroll), ImVec2(posX + size.x - 10, posY + 20 - scroll), (hovered ? ImGui::GetColorU32(ImVec4(1, 1, 1, 1)) : ImGui::GetColorU32(tasker::Colors::green)), 1.5);
            draw->AddLine(ImVec2(posX + size.x - 25, posY + 20 - scroll), ImVec2(posX + size.x - 10, posY + 5 - scroll), (hovered ? ImGui::GetColorU32(ImVec4(1, 1, 1, 1)) : ImGui::GetColorU32(tasker::Colors::green)), 1.5);

            // Increments the x offset to draw the next box over
            posX += spacing + size.x;

            // If the box is at the end of the row, reset the x offset, and increase y offset
            if (current_box % row_size == 0) {
                posY += spacing + size.y;
                posX = originalPosX;
            }

            // Increment the current box so it can detect a new row
            current_box++;

            // If the box is clicked, change the global stage variable to begin drawing a workspace, and set the connection to the selected databse
            if (picked) {
                connection = connections.get_database(i);
                stage = tasker::DisplayWindowStage::workspace_main;
            }

            // If the box is deleted, remove the databse from the json file, and initiate a refresh
            if(pushed) {
                tasker::remove_json_database(connections.get_database(i));
                refresh = true;
            }
        }

        // Draw the add button, to open the add screen
        ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x / 2) - 65, posY + spacing + size.y));
        if (ImGui::Button("Add", ImVec2(50, 50)) || add_connection) {
            add_connection = true;
            display_connection_add(add_connection, connections, refresh, statics);
        }
        // TODO fix positioning - ehhh they look fine
        ImGui::SetCursorPos(ImVec2((ImGui::GetWindowSize().x / 2) - 5, posY + spacing + size.y));
        refresh = ImGui::Button("Refresh", ImVec2(75, 50)) || refresh;

        ImGui::End();

    }
}

void draw_supertask(tasker::supertask* task, int& y, int& latestId, tasker::workspace& w, bool& refresh);
void create_new(tasker::workspace& w, bool& create_cat, float* colors, char* buffer, bool& refresh);
void manage_statuses(bool& manage_statuses, bool& refresh, tasker::workspace& w, int& latestId, char* buffer, float* colors);

void display_workspace(tasker::DisplayWindowStage& stage, int& latestId, bool& refresh, tasker::workspace& w, time_t& timer, tasker::workspace_statics& statics) {
    // Grab new data if a refresh is needed
    if(refresh) {
        w.fullRefresh();
    }

    // Set next drawn window to be fullscreen
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoCollapse;
    ImGui::SetNextWindowSize(ImGui::GetMainViewport()->Size);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->Pos);
    bool open = true;
    if (ImGui::Begin(w.getName(), &open, flags)) {
        // The y offset to begin drawing tables / categories
        int y = 100 - ImGui::GetScrollY();

        // The drawing of buttons to create/manage categories, statuses, or to refresh the screen
        ImGui::SetCursorPos(ImVec2(50, 50));
        if(ImGui::Button("New Category", ImVec2(150, 25)) || statics.create_cat) {
            // If going into the screen for the first time, fill the persistent text field with \0
            if(!statics.create_cat) std::fill(statics.new_category, statics.new_category + (MAX_STRING_LENGTH - 1), '\0');
            statics.create_cat = true;
            create_new(w, statics.create_cat, statics.new_color, statics.new_category, refresh);
        }
        ImGui::SameLine();
        if(ImGui::Button("Manage Statuses", ImVec2(150, 25)) || statics.manage_statuses) {
            if(!statics.manage_statuses) std::fill(statics.new_category, statics.new_category + (MAX_STRING_LENGTH - 1), '\0');
            statics.manage_statuses = true;
            manage_statuses(statics.manage_statuses, refresh, w, latestId, statics.new_category, statics.new_color);
        }
        ImGui::SameLine();
        if(ImGui::Button("Refresh", ImVec2(100, 25))) {
            refresh = true;
            time(&timer);
        }

        {
            // In a block to auto-destruct temp. This block displays the refresh message if the time since refresh is lower than 6 seconds
            time_t temp;
            time(&temp);
            if(abs(timer - temp) < 6) {
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0, 1, 0, 1), "Refreshed!");
            }
        }

        // Iterate through all supertasks and draw them
        for(tasker::supertask* s : *w.getSupers()) 
            draw_supertask(s, y, latestId, w, refresh);

        // Expand the scrollable area of the window
        ImGui::Dummy(ImVec2(0, y + 10));
    }
    ImGui::End();

    // If the user quits the workspace, return to the homescreen
    if(!open) {
        stage = tasker::DisplayWindowStage::pick_workspace;
        refresh = true;
    }
}

// Big chonky function, does the majority of the heavy lifting for the whole application
void draw_supertask(tasker::supertask* task, int& y, int& latestId, tasker::workspace& w, bool& refresh) {    // Variables needed for color styling and text
    constexpr double color_shift = 1.5;
    const float scroll = ImGui::GetScrollY();
    const bool isDark = ((task->getColor()->x + task->getColor()->y + task->getColor()->z) / 3) < 0.33;
    const ImColor main_color = isDark ? ImColor(ImVec4(1, 1, 1, 1)) : ImColor(ImVec4(0, 0, 0, 1));
    const ImColor accent = isDark ? ImColor(ImVec4(0, 0, 0, 1)) : ImColor(ImVec4(1, 1, 1, 1));

    ImDrawList* draw = ImGui::GetWindowDrawList();

    // Sets the color of text fields, and of text to their main color (either black or white), and the main color of the supertask
    ImGui::PushStyleColor(ImGuiCol_FrameBg, alphaShift(*task->getColor(), 0.25));
    ImGui::PushStyleColor(ImGuiCol_Text, main_color.Value);

    // If the overall supertask color is dark, draw a white outline to separate it from the background
    if(isDark) draw->AddRectFilled(ImVec2(48, y - 2), ImVec2(ImGui::GetWindowSize().x - 48, y + 37), ImColor(ImVec4(1, 1, 1, 1)), 5);

    // Draw the actual supertask banner
    draw->AddRectFilled(ImVec2(50, y - ImGui::GetScrollY()), ImVec2(ImGui::GetWindowSize().x - 50, y + 35 - ImGui::GetScrollY()), 
            ImGui::GetColorU32(*task->getColor()), 5);

    // Draw supertask label/text
    ImGui::SetCursorPos(ImVec2(75, y + 7));
    ImGui::TextUnformatted(task->getName());

    // Pop previous text color, and set it to white
    ImGui::PopStyleColor();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));

    // Create button to delete supertask
    ImGui::PushID(latestId++);
    ImGui::SetCursorPos(ImVec2(ImGui::GetWindowSize().x - 75, y + 10 - scroll));
    bool pushedd = ImGui::InvisibleButton("##delete", ImVec2(15, 15));
    bool hovered = ImGui::IsItemHovered();
    ImGui::PopID();

    // Draw delete button
    draw->AddLine(ImVec2(ImGui::GetWindowSize().x - 75, y + 10 - scroll), ImVec2(ImGui::GetWindowSize().x - 60, y + 25 - scroll), 
            (hovered ? ImGui::GetColorU32(accent.Value) : ImGui::GetColorU32(main_color.Value)), 1.5);
    draw->AddLine(ImVec2(ImGui::GetWindowSize().x - 75, y + 25 - scroll), ImVec2(ImGui::GetWindowSize().x - 60, y + 10 - scroll), 
            (hovered ? ImGui::GetColorU32(accent.Value) : ImGui::GetColorU32(main_color.Value)), 1.5);

    // Draw collapse arrow, in proper orientation according to the state of if it is collapsed
    if(!task->collapsed) draw->AddTriangleFilled(ImVec2(55, y + 15 - scroll), ImVec2(62.5, y + 25 - scroll), ImVec2(70, y + 15 - scroll), main_color);
    else draw->AddTriangleFilled(ImVec2(60, y + 10 - scroll), ImVec2(60, y + 25 - scroll), ImVec2(67.5, y + 17.5 - scroll), main_color);

    // Create button for collapsing task
    ImGui::SetCursorPos(ImVec2(55, y + 10));
    bool pushed = ImGui::InvisibleButton(std::string("##task_opening_").append(task->getName()).c_str(), ImVec2(15, 15));

    // If the collapse arrow is hovered, create darkened circle around it to indicate it is hovered
    if(ImGui::IsItemHovered()) draw->AddCircleFilled(ImVec2(62.5, y + 17.5 - scroll), 9, ImColor(ImVec4(0.5, 0.5, 0.5, 0.5)));

    // Only draw rest of supertask if it is not collapsed
    if(!task->collapsed) {
        // Always increment y to avoid drawing collisions
        y += 40;

        // Iterate through all tasks in supertask
        for(tasker::task* t : *task->getTasks()) {

            // Draw the main body of the task, and decorative rectangle on the left side
            draw->AddRectFilled(ImVec2(50, y - ImGui::GetScrollY()), ImVec2(ImGui::GetWindowSize().x - 50, y + 35 - ImGui::GetScrollY()), ImColor(ImVec4(0.25, 0.25, 0.25, 1)), 5);
            draw->AddRectFilled(ImVec2(50, y - ImGui::GetScrollY()), ImVec2(55, y + 35 - ImGui::GetScrollY()), ImGui::GetColorU32(*task->getColor()));

            // Boolean to detect if any changes have been made to a task, and update server if any have been detected
            bool changed = false;

            // Draw main body of task, and detect if it has been selected for updating
            ImGui::SetCursorPos(ImVec2(60, y + 3));
            ImGui::PushItemWidth((ImGui::GetWindowSize().x - 120) / 2);
            bool taskChanged = ImGui::InputText(std::string("##task_input_").append(std::to_string(latestId++)).c_str(), 
                    &t->task_write_buf[0], MAX_STRING_LENGTH, ImGuiInputTextFlags_EnterReturnsTrue);
            if(taskChanged || ImGui::IsItemActive()) {
                w.setTaskTask(t, t->task_write_buf);
                changed = true;
            }
            ImGui::PopItemWidth();

            // Draw text field for people, and detect any changes
            ImGui::SetCursorPos(ImVec2((((ImGui::GetWindowSize().x - 125) / 2) + 70), y + 3));
            ImGui::PushItemWidth((ImGui::GetWindowSize().x - 100) / 3);
            bool peopleChanged = ImGui::InputText(std::string("##task_people_").append(std::to_string(latestId++)).c_str(), 
                    &t->people_write_buf[0], MAX_STRING_LENGTH, ImGuiInputTextFlags_EnterReturnsTrue);
            if(peopleChanged || ImGui::IsItemActive()) {
                w.setTaskPeople(t, t->people_write_buf);
                changed = true;
            }
            ImGui::PopItemWidth();

            // Drawing block for selecting status
            // Temporarily set the color to whatever the selected status is
            ImGui::PushStyleColor(ImGuiCol_FrameBg, *t->getStatus()->getColor());

            // Set the hovered color to slightly lighter than the non-hovered color for user feedback
            {
                ImVec4 col = *t->getStatus()->getColor();
                ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, alphaShift((col * color_shift), 0.8));
            }

            // Begin drawing of the actual combo
            ImGui::SetCursorPos(ImVec2((((ImGui::GetWindowSize().x - 120) * (5.0 / 6.0)) + 80), y + 3));
            ImGui::PushItemWidth(((ImGui::GetWindowSize().x - 100) / 6) - 50);

            // Seperate changed flag for the combo
            bool edited = false;
            if(ImGui::BeginCombo(std::string("##status_selector_").append(std::to_string(latestId++)).c_str(), t->getStatus()->getName(), ImGuiComboFlags_NoArrowButton)) {
                bool first = true;
                for(size_t i = 0; i < w.getStati()->size(); i++) {
                    bool selected = t->getStatus() == w.getStati()->at(i);

                    // Use buttons inside the combo to achieve solid color, selectable object that will immediately close the combo
                    ImGui::PushStyleColor(ImGuiCol_Button, alphaShift(*w.getStati()->at(i)->getColor(), (selected ? 0.9 : 0.8)));
                    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, alphaShift(*w.getStati()->at(i)->getColor(), 0.95));
                    ImGui::PushStyleColor(ImGuiCol_ButtonActive, alphaShift(*w.getStati()->at(i)->getColor(), 1));
                    if(ImGui::Button(w.getStati()->at(i)->getName(), ImVec2(ImGui::GetItemRectMax().x - ImGui::GetItemRectMin().x - (first ? 10 : 0), 0))) {
                        w.setTaskStatus(t, w.getStati()->at(i));
                        edited = true;
                    }
                    if(first) first = false;
                    ImGui::PopStyleColor(3);

                    // If a button is pushed, then a status must have been selected
                    if(selected) ImGui::SetItemDefaultFocus();
                }

                ImGui::EndCombo();
            }

            // Draw/create objects for deleting task
            ImGui::SetCursorPos(ImVec2(ImGui::GetWindowSize().x - 75, y + 10 - scroll));
            bool toDelete = ImGui::InvisibleButton(std::string("##").append(std::to_string(latestId++)).c_str(), ImVec2(15, 15)); 
            bool toDeleteHovered = ImGui::IsItemHovered();
            draw->AddLine(ImVec2(ImGui::GetWindowSize().x - 75, y + 10 - scroll), ImVec2(ImGui::GetWindowSize().x - 60, y + 25 - scroll), (toDeleteHovered ? ImGui::GetColorU32(ImVec4(1, 1, 1, 1)): ImGui::GetColorU32(ImVec4(0, 0, 0, 1))), 1.5);
            draw->AddLine(ImVec2(ImGui::GetWindowSize().x - 75, y + 25 - scroll), ImVec2(ImGui::GetWindowSize().x - 60, y + 10 - scroll), (toDeleteHovered ? ImGui::GetColorU32(ImVec4(1, 1, 1, 1)): ImGui::GetColorU32(ImVec4(0, 0, 0, 1))), 1.5);

            

            // OR the text fields and combo to see total change count
            changed = changed || edited;

            // This schema is so imgui can focus on the window and close the combo, not for updating mysql
            if(edited) ImGui::SetWindowFocus(w.getName());

            // If a change is detected, update mysql server
            t->wasSelected = changed;
            ImGui::PopStyleColor();
            ImGui::PopStyleColor();

            if(toDelete) {
                w.dropTask(t);
                refresh = true;
            }

            y += 40;
        }

        // All of this is a copy - paste of the above code, to draw an additional task for adding a new task
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
        ImGui::PushStyleColor(ImGuiCol_Button, *task->getColor());
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, alphaShift(main_color.Value, 0.1));
        draw->AddRectFilled(ImVec2(50, y - ImGui::GetScrollY()), ImVec2(ImGui::GetWindowSize().x - 50, y + 35 - ImGui::GetScrollY()), ImColor(ImVec4(0.25, 0.25, 0.25, 1)), 5);

        ImGui::SetCursorPos(ImVec2(60, y + 3));
        ImGui::PushItemWidth((ImGui::GetWindowSize().x - 120) / 2);
        ImGui::InputText(std::string("##task_input_").append(std::to_string(latestId++)).c_str(), task->task_write_buf, MAX_STRING_LENGTH, ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::IsItemActive();
        ImGui::PopItemWidth();

        ImGui::SetCursorPos(ImVec2((((ImGui::GetWindowSize().x - 125) / 2) + 70), y + 3));
        ImGui::PushItemWidth((ImGui::GetWindowSize().x - 100) / 3);
        ImGui::InputText(std::string("##task_people_").append(std::to_string(latestId++)).c_str(), task->people_write_buf, MAX_STRING_LENGTH, ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::PopItemWidth();

        ImVec4 taskColor = (task->new_status == nullptr ? ImVec4(0, 0, 0, 1) : *task->new_status->getColor());
        ImGui::PushStyleColor(ImGuiCol_FrameBg, taskColor);
        float color_shift = 1.5f;
        ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, alphaShift(taskColor * color_shift, 0.8));
        ImGui::SetCursorPos(ImVec2((((ImGui::GetWindowSize().x - 120) * (5.0 / 6.0)) + 80), y + 3));
        ImGui::PushItemWidth(((ImGui::GetWindowSize().x - 100) / 6) - 50);
        if(ImGui::BeginCombo(std::string("##status_selector_").append(std::to_string(latestId++)).c_str(), (task->new_status == nullptr ? 
                        "None" : task->new_status->getName()), ImGuiComboFlags_NoArrowButton)) {
            bool first = true;
            for(size_t i = 0; i < w.getStati()->size(); i++) {
                bool selected = (task->new_status == nullptr ? w.getStatusFromString("None") : task->new_status) == w.getStati()->at(i);
                ImGui::PushStyleColor(ImGuiCol_Button, alphaShift(*w.getStati()->at(i)->getColor(), (selected ? 0.9 : 0.8)));
                ImGui::PushStyleColor(ImGuiCol_ButtonHovered, alphaShift(*w.getStati()->at(i)->getColor(), 0.95));
                ImGui::PushStyleColor(ImGuiCol_ButtonActive, alphaShift(*w.getStati()->at(i)->getColor(), 1));
                if(ImGui::Button(w.getStati()->at(i)->getName(), ImVec2(ImGui::GetItemRectMax().x - ImGui::GetItemRectMin().x - (first ? 10 : 0), 0))) {
                    task->new_status = w.getStati()->at(i);
                }
                if(first) first = false;
                ImGui::PopStyleColor();
                ImGui::PopStyleColor();
                ImGui::PopStyleColor();
                if(selected) ImGui::SetItemDefaultFocus();
            }
            ImGui::EndCombo();
        }

        ImGui::SetCursorPos(ImVec2(ImGui::GetWindowSize().x - 82, y + 3));
        bool selected = ImGui::InvisibleButton(std::string("##").append(std::to_string(latestId++)).c_str(), ImVec2(25, 25));
        bool hovered = ImGui::IsItemHovered();
        draw->AddLine(ImVec2(ImGui::GetWindowSize().x - 75, y + 22 - scroll), ImVec2(ImGui::GetWindowSize().x - 70, y + 27 - scroll), ImGui::GetColorU32(!hovered ? ImVec4(0, 0, 0, 1) : ImVec4(1, 1, 1, 1)), 2.0f);
        draw->AddLine(ImVec2(ImGui::GetWindowSize().x - 70, y + 27 - scroll), ImVec2(ImGui::GetWindowSize().x - 60, y + 5 - scroll), ImGui::GetColorU32(!hovered ? ImVec4(0, 0, 0, 1) : ImVec4(1, 1, 1, 1)), 2.0f);

        if(selected) {
            w.createTask(task);
            refresh = true;
        }
        ImGui::PopStyleColor(5);
        // End creating new task
    }

    // If the collapse button is pushed, update the state variable
    if(pushed) task->collapsed = !task->collapsed;

    // Pop remaining styles, and incrememnt y for the next supertask
    ImGui::PopStyleColor(2);
    y += 75;

    // If the delete was pushed, remove the supertask from the mysql server, and initiate refresh 
    if(pushedd) {
        w.dropCategory(task);
        refresh = true;
    }
}

// Screen for creating a new supertask
void create_new(tasker::workspace& w, bool& create_cat, float* colors, char* buffer, bool& refresh) {
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;
    ImGui::SetNextWindowSize(ImVec2(500, 500));
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + (viewport->Size.x / 2) - 250, viewport->Pos.y + (viewport->Size.y / 2) - 250));

    if(ImGui::Begin("Create New Category", &create_cat, flags)) {
        // Create a color picker to determine new color
        ImGui::ColorPicker3("New Color", colors);
        ImGui::TextUnformatted("Category name: ");
        ImGui::SameLine();
        // Text input for supertask name
        ImGui::InputText("##new_cat_name", buffer, MAX_STRING_LENGTH);

        // Draw an "ok" button to create task when ready
        bool create;
        constexpr int button_size = 100;
        ImGui::SetCursorPosX((ImGui::GetWindowSize().x - button_size) / 2);
        create = ImGui::Button("Create", ImVec2(button_size, 25));

        // If the button is pushed, actually create the task, update server, and initiate refresh
        if(create) {
            w.createCategory(buffer, ImVec4(colors[0], colors[1], colors[2], 1.0f));
            refresh = true;
            create_cat = false;
        }

        // Determine if create window is still in focus
        create_cat = create_cat && ImGui::IsWindowFocused();
        ImGui::End();
    }
}

void manage_statuses(bool& manage_statuses, bool& refresh, tasker::workspace& w, int& latestId, char* buffer, float* colors) {
    ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize;
    ImGui::SetNextWindowSize(ImVec2(500, 500));
    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2(viewport->Pos.x + (viewport->Size.x / 2) - 250, viewport->Pos.y + (viewport->Size.y / 2) - 250));

    if(ImGui::Begin("Mange Statuses", &manage_statuses, flags)) {
        // Iterate through all statuses to display them
        for(tasker::status* s : *w.getStati()) {
            if(s->getName() == string("None")) continue;
            // Display solid color button for the status name, easier than using basic level drawing api
            ImGui::SetCursorPosX(45);
            ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetColorU32(*s->getColor()));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImGui::GetColorU32(*s->getColor()));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImGui::GetColorU32(*s->getColor()));
            ImGui::Button(string(s->getName()).append("##").append(std::to_string(1)).c_str(), ImVec2(200, 50));
            ImGui::PopStyleColor();
            ImGui::PopStyleColor();

            // Actual removing button
            ImGui::SameLine();
            ImGui::SetCursorPosX(255);
            if(ImGui::Button(std::string("Remove##").append(std::to_string(latestId++)).c_str(), ImVec2(200, 50))) {
                w.dropStatus(s);
                refresh = true;
                manage_statuses = false;
            }
            ImGui::PopStyleColor();
        }

        // Section to display area to create new button
        ImGui::NewLine();
        ImGui::ColorPicker3("New Color", colors);

        ImGui::TextUnformatted("Status Name: ");
        ImGui::PushID(latestId++);
        ImGui::InputText("##", buffer, MAX_STRING_LENGTH);
        ImGui::PopID();

        ImGui::SetCursorPosX(45);
        if(ImGui::Button("Create Status", ImVec2(410, 50))) {
            w.createStatus(buffer, ImVec4(colors[0], colors[1], colors[2], 1.0f));
            refresh = true;
        }
        manage_statuses = manage_statuses && ImGui::IsWindowFocused();
        ImGui::End();
    }
}
