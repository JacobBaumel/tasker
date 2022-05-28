#ifndef DISPLAY_WINDOWS_H
#define DISPLAY_WINDOWS_H
#include "jsonstuff.h"
#include <map>

namespace tasker {
    enum class DisplayWindowStage {
        pick_workspace,
        workspace_main,
    };

    struct input_field_manager {
        std::map<std::string, char[]> fields;
    };
};

void display_workspace(tasker::json_sql_connection& connection, tasker::DisplayWindowStage& stage, int& latestId);
void display_worskapce_selection(tasker::json_sql_connection& connection, tasker::DisplayWindowStage& stage, int& latestId);

#endif