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
        std::string name;
        ImColor color;
    };

    struct task {
        status* statuss;
        std::string taskk;
        std::string date;
        std::string people;
    };

    struct supertask {
        std::vector<task*> tasks;
        ImColor color;
        std::string name;
    };

    struct workspace {
        std::vector<status*> stati;
        std::vector<supertask*> tasks;
        std::string name;

        ~workspace() {
            for(status* s : stati) delete s;
            for(supertask* t : tasks) delete t;
        }

        status* get_status(const std::string& name) {
            for(status* s : stati) if(s->name == name) return s;
            return nullptr;
        }
    };

    return_code has_open_connection();
    return_code set_connection(json_sql_connection& conn);
    return_code does_workspace_exist(const std::string& name);
    return_code create_workspace(const std::string& name);
    return_code get_data(workspace& config);
    return_code set_schema(const std::string& schema);
}
#endif