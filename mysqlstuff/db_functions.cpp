#include "mysql_connection.h"
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include "db_functions.h"
#include "jsonstuff.h"

namespace tasker {

    sql::Connection* connection = NULL;
    json_sql_connection previous;

    /*
    Returns whether a connection has been opened
    returns:
        return_code::True if connection is opened
        return_code::False if connection is NULL or not open
        return_code::Error if failure
    */
    return_code has_open_connection() {
        try {
            if(connection == NULL) return False;
            return connection->isClosed() ? False : True;
        } catch(sql::SQLException& e) {
            std::cerr << "Error determining if connection is open!" << std::endl << e.what() << std::endl;
            return Error;
        }
    }

    /*
    Sets and opens the provided connection
    returns:
        return_code::True if successful
        return_code::False if provided connection is the same as the already existing connection
        return_code::Error if failure
    */
    return_code set_connection(json_sql_connection& conn) {
        if(conn == previous) return False;
        try {
            connection = get_driver_instance()->connect("tcp://" + conn.ip + ":" + std::to_string(conn.port), conn.username, conn.password);
            if(!connection->isValid() || connection->isClosed()) return Error;
        } catch(sql::SQLException& e) {
            std::cerr << "Could not connect to server!" << std::endl << e.what() << std::endl;
            return Error;
        }

        previous = conn;

        return True;
    }
    
    /*
    Takes in a string of workspace name to validate
    returns:
        return_code::TRUE if the name is unavailable
        return_code::FALSE if name is available
        return_code::ERROR if an error occured
    */
    return_code does_workspace_exist(const std::string& name) {
        if(connection == NULL || connection->isClosed()) return Error;
        sql::Statement* stmt;
        sql::ResultSet* result;
        try {
            stmt = connection->createStatement();
            result = stmt->executeQuery("show databases like '" + name + "'");
            int num_results = result->rowsCount();
            delete stmt;
            delete result;
            return num_results == 1 ? True : False;
        } catch(sql::SQLException& e) {
            std::cerr << "Error detecting databases!" << std::endl << e.what() << std::endl;
            delete stmt;
            delete result;
            return Error;
        }
    }
}