#ifndef MYSQL_FUNCTIONS_H
#define MYSQL_FUNCTIONS_H
#include "mysql_connection.h"
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <string>

namespace tasker {
    struct workspace_properties {
        std::string db_name;
        std::string org_name;
        std::string workspace_name;
        std::string description;
    };

    struct Color {
        std::uint8_t red;
        std::uint8_t green;
        std::uint8_t blue;
    };

    //basic stuff
    sql::ResultSet* show_available_databases(sql::Connection* connection);
    bool create_workspace(sql::Connection* connection, workspace_properties& properties);

    //Status modifiers
    bool create_status(sql::Connection* connection, const std::string& name, const int r, const int g, const int b, const std::string& description);
    bool modify_status_name(sql::Connection* connection, int id, std::string& name);
    //set color number to negative to use what is already there
    bool modify_status_color(sql::Connection* connection, int id, Color& color);
    bool modify_status_description(sql::Connection* connection, int id, std::string& description);
    bool delete_status(sql::Connection* connection, int id);

    //People modifiers
    bool create_person(sql::Connection* connection, const std::string& name, const std::string& role);
    bool modify_person_name(sql::Connection* connection, int id, const std::string& name);
    bool modify_person_role(sql::Connection* connection, int id, const std::string& role);
    bool delete_person(sql::Connection* connection, int id);

    //Supertask modifiers
    bool create_supertask(sql::Connection* connection, const std::string& name, const Color& color);
}

#endif