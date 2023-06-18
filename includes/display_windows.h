#ifndef DISPLAY_WINDOWS_H
#define DISPLAY_WINDOWS_H
#include "jsonstuff.h"
#include "imgui.h"
#include "structs.hpp"
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
        char schema[128] = "";
        char ip[128] = "";
        int port = 3306;
        char username[128] = "";
        bool show_pass = false;
        char password[128] = "";
        int error1 = 0;
        int error2 = 0;
        int error3 = 0;
    };

    struct workspace_statics {
        bool create_cat = false;
        bool manage_statuses = false;
        char new_category[MAX_STRING_LENGTH];
        float new_color[3] = {0, 0, 0};
    };
};

void display_workspace(tasker::DisplayWindowStage& stage, int& latestId, bool& refresh, tasker::workspace& config, time_t& timer, tasker::workspace_statics& statics);
void display_worskapce_selection(tasker::json_database& connection, tasker::DisplayWindowStage& stage, int& latestId, bool& refresh, tasker::database_array& connections, bool& add_connection, tasker::connection_add_statics& statics);

#endif
