#ifndef DISPLAY_WINDOWS_H
#define DISPLAY_WINDOWS_H

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

#endif