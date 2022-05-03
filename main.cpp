#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <cppconn/connection.h>
#include <cppconn/driver.h>
#include "includes/jsonstuff.h"
#include <fstream>

namespace tasker {
    enum class DisplayWindow {
        add_database,
        pick_workspace,
        workspace_main,
    };
};

void display_connection_add(tasker::DisplayWindow& stage);
void display_workspace(tasker::json_sql_connection& connection);
void display_worskapce_selection(tasker::json_sql_connection& connection);

int main() {

    {if(!std::ifstream("config.json").good()) {std::ofstream("config.json") << "{\"connections\": []}";}}

    //GLFW and OpenGL setup
    if(!glfwInit()) {
        std::cerr << "Could not initialize OpenGL!" << std::endl;
        return -1;
    }
    

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);

    GLFWwindow* window = glfwCreateWindow(1920, 1080, "Tasker", nullptr, nullptr);

    if(!window) {
        std::cerr << "Could not create a window!" << std::endl;
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");
    ImVec4 clear_color = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    bool has_picked_workspace = false;
    tasker::DisplayWindow stage = tasker::DisplayWindow::add_database;

    ImFont* ubuntu = io.Fonts->AddFontFromFileTTF("fonts/Ubuntu-Light.ttf", 20);

    //Main window loop
    while(!glfwWindowShouldClose(window)) {
        
        //setting up new rendering frame
        glfwPollEvents();

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::StyleColorsDark();
        ImGui::GetStyle().Colors[ImGuiCol_TitleBgActive] = ImVec4(0.263, 0.749, 0.004, 1);

        //actual program logic
        switch(stage) {
            case tasker::DisplayWindow::add_database:
                display_connection_add(stage);
                break;

            case tasker::DisplayWindow::pick_workspace:
                break;

            case tasker::DisplayWindow::workspace_main:
                break;
        }

        //closing rendering frame, and drawing it on screen
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(window);
        
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

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

void display_worskapce_selection(tasker::json_sql_connection& connection) {

}
/*

MySQL info scheme:

Entire database represents a work environment

Table Metadata in string string key-value pairs:
Organization name
workspace name
workspace description
table names and color to be displayed as

Table People
stores the person with an:
id
name
role

Table Stati:
Status name
color (hex)
description

Task tables:
each overall task has its own table, which stores subtasks with:
name
person/people to complete
due date
status
comments
subtasks have an appearence order

Table Archive:
has each archived task and what supertask they originally belonged to

General functionality:
Display all supertasks on main page
each supertask can have subtasks, of which have name, status, description, comments, people, and due date
statuses are colored boxes, and can be changed and named in the menu
supertasks are colored
all colors are in hex form, and a color picker tool will be available
can add people and dates to tasks
supertasks are collapsable
the top will show organization, workspace name, along with description
ability to select from multiple workspaces (databases) within one mysql server
ability to use date picker to pick due dates

Customizability options for single user only (persistent):
Use custom font
Change background color
supertasks start alphabetical
subtasks are as ordered in 

Needed mysql functions:
show all available databases for workspace options - done
create new workspace - done
initialize workspace (metadata) and tables people and stati - done
verify workspace integrity (all needed tables are present)
create/modify/delete status - done
create/modify/delete people - done
create/modify/delete supertask
create/modify/delete subtasks
get all supertask tables
get specific supertask table
refresh all available information
refresh only tables
refresh specific table
change names of supertasks
change appearence order of subtasks
*/