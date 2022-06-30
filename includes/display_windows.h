#ifndef DISPLAY_WINDOWS_H
#define DISPLAY_WINDOWS_H
#include "jsonstuff.h"
#include "db_functions.h"
#include "imgui.h"
#include <vector>

namespace tasker {
    enum class DisplayWindowStage {
        pick_workspace,
        workspace_main,
    };

    struct connection_add_statics {
        std::vector<std::string>* connection_options = NULL;
        const char* current = NULL;
        char workspace_name[64] = "";
        tasker::return_code success1 = tasker::return_code::None;
        char schema[128] = "";
        tasker::return_code success2 = tasker::return_code::None;
        char ip[128] = "";
        int port = 3306;
        char username[128] = "";
        bool show_pass = false;
        char password[128] = "";
        tasker::return_code success3 = tasker::return_code::None;
    };
};

void display_workspace(tasker::json_database& connection, tasker::DisplayWindowStage& stage, int& latestId, bool& refresh, tasker::workspace& config);
void display_worskapce_selection(tasker::json_database& connection, tasker::DisplayWindowStage& stage, int& latestId, bool& refresh, tasker::database_array& connections, bool& add_connection, tasker::connection_add_statics& statics);

#endif