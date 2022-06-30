#include "mysql_connection.h"
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include "db_functions.h"
#include "jsonstuff.h"
#include "display_windows.h"
#include <map>

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
            if(connection == NULL) return tasker::return_code::False;
            return connection->isClosed() ? tasker::return_code::False : tasker::return_code::True;
        } catch(sql::SQLException& e) {
            std::cerr << "Error determining if connection is open!" << std::endl << e.what() << std::endl;
            return tasker::return_code::Error;
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
        if(conn == previous) return tasker::return_code::False;
        try {
            connection = get_driver_instance()->connect("tcp://" + conn.ip + ":" + std::to_string(conn.port), conn.username, conn.password);
            if(!connection->isValid() || connection->isClosed()) return tasker::return_code::Error;
        } catch(sql::SQLException& e) {
            std::cerr << "Could not connect to server!" << std::endl << e.what() << std::endl;
            return tasker::return_code::Error;
        }

        previous = conn;

        return tasker::return_code::True;
    }
    
    /*
    Takes in a string of workspace name to validate
    returns:
        return_code::TRUE if the name is unavailable
        return_code::FALSE if name is available
        return_code::ERROR if an error occured
    */
    return_code does_workspace_exist(const std::string& name) {
        if(has_open_connection() != tasker::return_code::True) return tasker::return_code::Error;
        sql::Statement* stmt;
        sql::ResultSet* result;
        try {
            stmt = connection->createStatement();
            result = stmt->executeQuery("show databases like '" + name + "'");
            int num_results = result->rowsCount();
            delete stmt;
            delete result;
            return num_results >= 1 ? tasker::return_code::True : tasker::return_code::False;
        } catch(sql::SQLException& e) {
            std::cerr << "Error detecting databases!" << std::endl << e.what() << std::endl;
            delete stmt;
            delete result;
            return tasker::return_code::Error;
        }
    }

    return_code create_workspace(const std::string& name) {
        if(has_open_connection() != tasker::return_code::True) return tasker::return_code::Error;
        sql::Statement* stmt = NULL;

        try {
            stmt = connection->createStatement();
            stmt->executeUpdate("create database " + name);
            connection->setSchema(name);
            delete stmt;
            stmt = connection->createStatement();
            stmt->executeUpdate("create table task_meta (name varchar(64), r int, g int, b int)");
            stmt->executeUpdate("create table stati (name varchar(64), r int, g int, b int)");
            delete stmt;
        } catch(sql::SQLException& e) {
            std::cerr << "Error creating workspace!" << std::endl << e.what() << std::endl;
            delete stmt;
            return tasker::return_code::Error;
        }

        return tasker::return_code::True;
    } 

    return_code set_schema(const std::string& schema) {
        if(has_open_connection() != tasker::return_code::True) return tasker::return_code::Error;
        try {
            connection->setSchema(schema);
        } catch(sql::SQLException& e) {
            std::cerr << "Error setting schema!" << std::endl << e.what() << std::endl;
            return tasker::return_code::Error;
        }

        return tasker::return_code::True;
    }

    return_code get_data(workspace& config) {
        if(has_open_connection() != tasker::return_code::True && connection->getSchema() != "") return tasker::return_code::Error;
        sql::Statement* stmt = NULL;
        sql::ResultSet* result = NULL;
        config = workspace();

        try {
            config.name = connection->getSchema();
            stmt = connection->createStatement();
            result = stmt->executeQuery("select * from stati");
            while(result->next())
                config.stati.push_back(new tasker::status{result->getString("name"), ImColor(result->getInt("r"), result->getInt("g"), result->getInt("b"))});
            
            delete result;
            std::map<std::string, ImColor> colors;
            result = stmt->executeQuery("select * from tasks_meta");
            while(result->next())
                colors.insert(std::pair<std::string, ImColor>(result->getString("name"), ImColor{result->getInt("r"), result->getInt("g"), result->getInt("b")}));
            delete result;
            
            result = stmt->executeQuery("show tables");
            while(result->next()) {
                std::string table = result->getString("Tables_in_" + connection->getSchema());
                if(table.substr(0, 5) != "task_") continue;
                tasker::supertask* task = new tasker::supertask();
                task->name = table.substr(5);
                task->color = colors.at(table.substr(5));
                
                sql::ResultSet* tasks = stmt->executeQuery("select * from " + table);
                while(tasks->next()) 
                    task->tasks.push_back(new tasker::task{config.get_status(tasks->getString("status")), tasks->getString("task"), tasks->getString("date"), tasks->getString("people")});
            
                config.tasks.push_back(task);
            }
            
            delete stmt;
        } catch(sql::SQLException& e) {
            std::cerr << "Error loading workspace!" << std::endl << e.what() << std::endl;
            delete stmt;
            delete result;
            return tasker::return_code::Error;
        }
        return tasker::return_code::True;
    }
}