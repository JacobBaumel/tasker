// All necessary includes for basic functions
// STD includes
#include <GLFW/glfw3.h>
#include <iostream>
#include <cppconn/connection.h>
#include <cppconn/driver.h>
#include <fstream>

// ImGui includes
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

// Tasker includes
#include "Colors.h"
#include "jsonstuff.h"

// These functions allow post and pre-rendering stuff to be done in multiple places, without needing a copy-paste
void pre_rendering();
void post_rendering(GLFWwindow* window);

// Global variable, used to determin when a full refresh is needed from server
bool refresh = true;

// Used to determine rising edge of refresh
bool prev_refresh = refresh;

int main() {

    // Start thread and initialize variables for server queryer thread
    // Checks if config file exists, and if not, create it
    {if(!std::ifstream("config.json").good()) {std::ofstream("config.json") << "{\"connections\": []}";}}

    //GLFW setup
    if(!glfwInit()) {
        std::cerr << "Could not initialize OpenGL!" << std::endl;
        return -1;
    }
    
    // Defining opengl version 3.0
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);

    // Creating window, values given for typical 1080p screen
    GLFWwindow* window = glfwCreateWindow(1920, 1080, "Tasker", nullptr, nullptr);

    if(!window) {
        std::cerr << "Could not create a window!" << std::endl;
        return -1;
    }

    // More imgui + opengl + glfw initialization
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // ImGui IO for detecting keyboard events
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    // Dark theme the best
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    // Necessary variables for whole progam runtime.
    tasker::json_database connection;

    // Enum for stage of program
    tasker::DisplayWindowStage stage = tasker::DisplayWindowStage::pick_workspace;

    // Better font than the default
    io.Fonts->AddFontFromFileTTF("fonts/Ubuntu-Light.ttf", 20);
    
    //Main window loop
    while(!glfwWindowShouldClose(window)) {
        //To determin what set of stuff to draw
        switch(stage) {
            case tasker::DisplayWindowStage::pick_workspace:
                {
                //These variables are outside of the secondary loop to persist between drawing loops, but to also be destroyed once the app is out of the picking stage
                tasker::database_array connections{};
                bool add_connection = false;
                tasker::connection_add_statics statics{};
                while(!glfwWindowShouldClose(window) && stage == tasker::DisplayWindowStage::pick_workspace) {
                    // Call pre and post rendering stuff inside loop
                    pre_rendering();

                    // latestId is reset each loop cycle, hence being redeclared in the loop
                    int latestId = 0;

                    // Master function responsible for drawing the whole picking screen
                    display_worskapce_selection(connection, stage, latestId, refresh, connections, add_connection, statics);
                    post_rendering(window);
                }
                break;
                }
            case tasker::DisplayWindowStage::workspace_main:
                // Set refresh to true in order to get data from server
                refresh = true;

                // Persistent workspace during workspace stage
                tasker::workspace config("");

                // Timer for refresh text
                time_t timer;
                while(!glfwWindowShouldClose(window) && stage == tasker::DisplayWindowStage::workspace_main) {
                    pre_rendering();
                    int latestId = 0;
                    
                    display_workspace(connection, stage, latestId, refresh, config, timer);
                    post_rendering(window);
                }
                break;
        }
    }

    // Closing of GLFW/Opengl stuff
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

void pre_rendering() {
    // Stuff ImGui needs to begin a rendering frame. No idea what it really does, dont need to know
    prev_refresh = refresh;

    glfwPollEvents();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    ImGui::StyleColorsDark();
    ImGui::GetStyle().Colors[ImGuiCol_TitleBgActive] = tasker::Colors::green;
    //ImGui::GetStyle().Colors[ImGuiCol_FrameBg] = tasker::Colors::background;
    ImGui::GetStyle().Colors[ImGuiCol_Button] = tasker::Colors::background;
    ImGui::GetStyle().Colors[ImGuiCol_ButtonHovered] = tasker::Colors::hovered;
    ImGui::GetStyle().Colors[ImGuiCol_ButtonActive] = tasker::Colors::active;
    ImGui::GetStyle().Colors[ImGuiCol_Tab] = tasker::Colors::hovered;
    ImGui::GetStyle().Colors[ImGuiCol_TabActive] = tasker::Colors::green;
    ImGui::GetStyle().Colors[ImGuiCol_TabHovered] = tasker::Colors::active;
}

void post_rendering(GLFWwindow* window) {
    // Stuff ImGui needs to end/draw a rendering frame. No idea what it really does, dont need to know
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
