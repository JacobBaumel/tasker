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

        status(const std::string& _name, const int& r, const int& g, const int& b) {
            std::copy(&_name[0], &_name[_name.length()], name);
            color = ImColor(r, g, b, 150);
        }
    };

    struct task {
        status* statuss;
        char taskk[256] = "";
        char date[256] = "";
        char people[256] = "";
        int pos;
        int id;
        bool wasSelected = false;

        task(status* _statuss, const std::string& _taskk, const std::string& _date, const std::string& _people, const int& _pos, const int& _id) {
            statuss = _statuss;
            pos = _pos;
            id = _id;
            std::copy(&_taskk[0], &_taskk[_taskk.length()], taskk);
            std::copy(&_date[0], &_date[_date.length()], date);
            std::copy(&_people[0], &_people[_people.length()], people);
        }
    };

    struct supertask {
        std::vector<task*> tasks;
        ImColor color;
        char name[256] = "";
        char display_name[256] = "";
        bool collapsed = false;

        supertask(const std::string& _name, const std::string& _display_name, const ImColor& _color) {
            std::copy(&_name[0], &_name[_name.length()], name);
            std::copy(&_display_name[0], &_display_name[_display_name.length()], display_name);
            color = _color;
        }
    };

    struct workspace {
        std::vector<status*> stati;
        std::vector<supertask*> tasks;
        char name[256] = "";
        char new_category[256] = "";
        float new_color[3] = {0, 0, 0};
        bool create_cat = false;
        bool manage_statuses = false;

        workspace(const std::string& _name) {
            std::copy(&_name[0], &_name[_name.length()], name);
        }

        ~workspace() {
            for(status* s : stati) delete s;
            for(supertask* t : tasks) delete t;
        }

        status* get_status(const std::string& name) {
            status* toreturn;
            for(status* s : stati) if(std::string(s->name) == name) return s;
            return nullptr;
        }
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
}
#endif