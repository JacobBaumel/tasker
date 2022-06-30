#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <iostream>
#include <cppconn/connection.h>
#include <cppconn/driver.h>
#include "includes/jsonstuff.h"
#include "includes/display_windows.h"
#include <fstream>
#include "Colors.h"

void pre_rendering();
void post_rendering(GLFWwindow* window);

bool refresh = true;
bool prev_refresh = refresh;

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

    tasker::json_database connection;
    bool has_picked_workspace = false;
    tasker::DisplayWindowStage stage = tasker::DisplayWindowStage::pick_workspace;
    tasker::DisplayWindowStage previous_stage = stage;

    ImFont* ubuntu = io.Fonts->AddFontFromFileTTF("fonts/Ubuntu-Light.ttf", 20);
    
    //Main window loop
    while(!glfwWindowShouldClose(window)) {
        switch(stage) {
            case tasker::DisplayWindowStage::pick_workspace:
                {
                tasker::database_array connections{};
                bool add_connection = false;
                tasker::connection_add_statics statics{};
                while(!glfwWindowShouldClose(window) && stage == tasker::DisplayWindowStage::pick_workspace) {
                    pre_rendering();
                    int latestId = 0;
                    display_worskapce_selection(connection, stage, latestId, refresh, connections, add_connection, statics);
                    post_rendering(window);
                }
                break;
                }
            case tasker::DisplayWindowStage::workspace_main:
                refresh = true;
                tasker::workspace config;
                while(!glfwWindowShouldClose(window) && stage == tasker::DisplayWindowStage::workspace_main) {
                    pre_rendering();
                    int latestId = 0;
                    display_workspace(connection, stage, latestId, refresh, config);
                    post_rendering(window);
                }
                break;
        }
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

void pre_rendering() {
    prev_refresh = refresh;

    glfwPollEvents();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::StyleColorsDark();
    ImGui::GetStyle().Colors[ImGuiCol_TitleBgActive] = tasker::Colors::green;
    ImGui::GetStyle().Colors[ImGuiCol_FrameBg] = tasker::Colors::background;
    ImGui::GetStyle().Colors[ImGuiCol_Button] = tasker::Colors::background;
    ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered] = tasker::Colors::hovered;
    ImGui::GetStyle().Colors[ImGuiCol_ButtonActive] = tasker::Colors::active;
    ImGui::GetStyle().Colors[ImGuiCol_Tab] = tasker::Colors::hovered;
    ImGui::GetStyle().Colors[ImGuiCol_TabActive] = tasker::Colors::green;
    ImGui::GetStyle().Colors[ImGuiCol_TabHovered] = tasker::Colors::active;
}

void post_rendering(GLFWwindow* window) {
    ImGui::Render();
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
    if(refresh && prev_refresh) refresh = false;
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
show all available databases for workspace options
create new workspace
initialize workspace (metadata) and tables people and stati
verify workspace integrity (all needed tables are present)
create/modify/delete status
create/modify/delete people
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