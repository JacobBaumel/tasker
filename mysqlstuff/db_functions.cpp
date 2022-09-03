#include "mysql_connection.h"
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include "db_functions.h"
#include "jsonstuff.h"
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
        if(has_open_connection() != tasker::return_code::True || connection->getSchema() == "") return tasker::return_code::Error;
        sql::Statement* stmt = NULL;
        sql::ResultSet* result = NULL;        
        
        try {
            config = workspace(connection->getSchema());
            stmt = connection->createStatement();
            result = stmt->executeQuery("select * from stati");
            while(result->next())
                config.stati.push_back(new tasker::status(result->getString("name"), result->getInt("r"), result->getInt("g"), result->getInt("b")));
            
            delete result;
            std::map<std::string, ImColor> colors;
            result = stmt->executeQuery("select * from tasks_meta");
            while(result->next())
                colors.insert(std::pair<std::string, ImColor>(result->getString("name"), ImColor{result->getInt("r"), result->getInt("g"), result->getInt("b"), 225}));
            delete result;
            
            result = stmt->executeQuery("show tables");
            while(result->next()) {
                std::string table = result->getString("Tables_in_" + connection->getSchema());
                if(table.substr(0, 5) != "task_") continue;
                std::string display = table.substr(5);
                for(int i = 0; i < display.length(); i++) if(display[i] == '_') display[i] = ' ';
                tasker::supertask* task = new tasker::supertask(table.substr(5), display, colors.at(table.substr(5)));
                
                sql::ResultSet* tasks = stmt->executeQuery("select * from " + table + " order by pos");
                while(tasks->next()) 
                    task->tasks.push_back(new tasker::task(config.get_status(tasks->getString("status")), tasks->getString("task"), tasks->getString("date"), tasks->getString("people"), tasks->getInt("pos"), tasks->getInt("idd")));
            
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

    return_code update_task(const std::string& table, task* t) {
        if(has_open_connection() != tasker::return_code::True || connection->getSchema() == "") return tasker::return_code::Error;
        sql::PreparedStatement* stmt;

        try {
            stmt = connection->prepareStatement("update task_" + table + " set task=?, status=?, people=?, date=?, pos=? where idd=?");
            //stmt->setString(1, "task_" + table); No idea why but the prepared statement doesnt like setString for the table name
            stmt->setString(1, t->taskk);
            stmt->setString(2, t->statuss->name);
            stmt->setString(3, t->people);
            stmt->setString(4, t->date);
            stmt->setInt(5, t->pos);
            stmt->setInt(6, t->id);
            stmt->execute();
        } catch(sql::SQLException& e) {
            std::cout << "Error updating server!" << std::endl << e.what() << std::endl;
            delete stmt;
            return return_code::Error;
        }

        return return_code::True;
    }

    return_code create_category(const std::string& _name, float* colors) {
        if(has_open_connection() != tasker::return_code::True || connection->getSchema() == "") return tasker::return_code::Error;
        sql::PreparedStatement* stmt;
        std::string name = _name;
        for(int i = 0; i < name.length(); i++) if(isspace(name[i])) name[i] = '_';

        try {
            stmt = connection->prepareStatement("create table task_" + name + "(task varchar(256), status varchar(64), people varchar(256), date varchar(16), pos int DEFAULT 0, idd int NOT NULL AUTO_INCREMENT, primary key(idd))");
            stmt->executeUpdate();
            delete stmt;

            stmt = connection->prepareStatement("insert into tasks_meta(name, r, g, b) values(?, ?, ?, ?)");
            stmt->setString(1, name);
            stmt->setInt(2, (int) (255 * colors[0]));
            stmt->setInt(3, (int) (255 * colors[1]));
            stmt->setInt(4, (int) (255 * colors[2]));
            stmt->executeUpdate();
            delete stmt;
        } catch(sql::SQLException& e) {
            std::cout << "Error creating new category!" << std::endl << e.what() <<std::endl;
            delete stmt;
            return return_code::Error;
        }

        return return_code::True;
    }

    return_code drop_category(const std::string& category) {
        if(has_open_connection() != tasker::return_code::True || connection->getSchema() == "") return tasker::return_code::Error;
        sql::Statement* stmt;

        try {
            stmt = connection->createStatement();
            stmt->executeUpdate("drop table task_" + category);
            stmt->executeUpdate("delete from tasks_meta where name=\"" + category + '"');
            delete stmt;
        } catch(sql::SQLException& e) {
            std::cout << "Error deleting category!" << std::endl << e.what() << std::endl;
            delete stmt;
            return return_code::Error;
        }

        return return_code::True;
    }

    return_code remove_status(const status* s, workspace& workspace) {
        if(has_open_connection() != tasker::return_code::True || connection->getSchema() == "") return tasker::return_code::Error;
        sql::Statement* stmt;

        try {
            stmt = connection->createStatement();
            stmt->executeUpdate("delete from stati where name=\"" + std::string(s->name) + "\"");
            for(supertask* tasks : workspace.tasks) {
                for(task* t : tasks->tasks) {
                    if(t->statuss == s) {
                        t->statuss = workspace.get_status("None");
                        update_task(tasks->name, t);
                    }
                }
            }
            delete stmt;
        } catch(sql::SQLException& e) {
            std::cout << "Error removing status!" << std::endl << e.what() << std::endl;
            delete stmt;
            return return_code::Error;
        }

        return return_code::True;
    }
}