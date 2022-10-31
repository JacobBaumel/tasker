#ifndef DB_FUNCTIONS_H
#define DB_FUNCTIONS_H

#include "jsonstuff.h"
#include "mysql_connection.h"
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include "imgui.h"
#include <ostream>

namespace tasker {
    enum class return_code {
        Error = -1,
        False = 0,
        True = 1,
        None = 2,
    };

    struct status {
        char name[256] = "";
        ImColor color;

        status(const std::string& _name, const int& r, const int& g, const int& b);
    };

    struct task {
        status* statuss;
        char taskk[256] = "";
        char date[256] = "";
        char people[256] = "";
        int pos;
        int id;
        bool wasSelected = false;

        task(status* _statuss, const std::string& _taskk, const std::string& _date, const std::string& _people, const int& _pos, const int& _id);
    };

    struct supertask {
        std::vector<task*> tasks;
        task* newTask;
        ImColor color;
        char name[256] = "";
        char display_name[256] = "";
        bool collapsed = false;

        supertask(const std::string& _name, const std::string& _display_name, const ImColor& _color);
    };

    struct workspace {
        std::vector<status*> stati;
        std::vector<supertask*> tasks;
        char name[256] = "";
        char new_category[256] = "";
        float new_color[3] = {0, 0, 0};
        bool create_cat = false;
        bool manage_statuses = false;

        workspace(const std::string& _name);

        ~workspace();

        status* get_status(const std::string& name);
    };

    return_code has_open_connection();
    return_code set_connection(json_sql_connection& conn);
    return_code does_workspace_exist(const std::string& name);
    return_code create_workspace(const std::string& name);
    return_code get_data(workspace& config);
    return_code set_schema(const std::string& schema);
    return_code update_task(const std::string& table, task* t);
    return_code create_category(const std::string& name, float* color);
    return_code drop_category(const std::string& category);
    return_code remove_status(const status* s, workspace& workspace);
    return_code create_status(const std::string& name, const int& r, const int& g, const int& b);
    return_code delete_task(const int& id, const std::string& super);
    return_code create_task(const task* task, const std::string& table);
}
#endif