#ifndef DB_FUNCTIONS_H
#define DB_FUNCTIONS_H

#include "jsonstuff.h"
#include "mysql_connection.h"
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>

namespace tasker {
    enum class return_code {
        Error = -1,
        False = 0,
        True = 1,
        None = 2,
    };

    return_code has_open_connection();
    return_code set_connection(json_sql_connection& conn);
    return_code does_workspace_exist(const std::string& name);
    return_code create_workspace(const std::string& name);
}

#endif