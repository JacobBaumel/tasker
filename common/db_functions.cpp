#include "mysql_connection.h"
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include "db_functions.h"
#include "jsonstuff.h"
#include <map>
#include <memory>

namespace tasker {
    sql::Connection* connection = NULL;
    json_sql_connection previous;

    /*
    Returns whether a connection has been opened by testing the value of the connection
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
    Sets and opens the provided connection. Tests the login information to ensure the connection is valid
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
    Takes in a string of workspace name to validate by determining if a database exists with the provided name in the mysql server
    returns:
        return_code::TRUE if the name is unavailable
        return_code::FALSE if name is available
        return_code::ERROR if an error occured
    */
    return_code does_workspace_exist(const std::string& name) {
        if(has_open_connection() != tasker::return_code::True) return tasker::return_code::Error;
        sql::Statement* stmt = nullptr;
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

    /*
    Creates a workspace on the server with the base necessities. These include:
      * Creating the database itself
      * Creating the task metadata table
      * Creating the stati table
      * Adding the null/none status to the database
    
    */
    return_code create_workspace(const std::string& name) {
        if(has_open_connection() != tasker::return_code::True) return tasker::return_code::Error;
        sql::Statement* stmt = NULL;

        try {
            stmt = connection->createStatement();
            stmt->executeUpdate("create database " + name);
            connection->setSchema(name);
            delete stmt;
            stmt = connection->createStatement();
            stmt->executeUpdate("create table tasks_meta (name varchar(64), r int, g int, b int)");
            stmt->executeUpdate("create table stati (name varchar(64), r int, g int, b int)");
            stmt->executeUpdate("insert into stati(name, r, g, b) values (\"None\", 0, 0, 0)");
            delete stmt;
        } catch(sql::SQLException& e) {
            std::cerr << "Error creating workspace!" << std::endl << e.what() << std::endl;
            delete stmt;
            return tasker::return_code::Error;
        }

        return tasker::return_code::True;
    } 

    /*
    Sets the workspace in the server to use
    */
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

    /*
    Get all data from the workspace and load it into memory
    */
    return_code get_data(workspace& config) {
        if(has_open_connection() != tasker::return_code::True || connection->getSchema() == "") return tasker::return_code::Error;
        sql::Statement* stmt = NULL;
        sql::ResultSet* result = NULL;        
        
        try {
            // Construct the workspace struct wuth the selected schema name
            config = workspace(connection->getSchema());
            stmt = connection->createStatement();

            // Load status options into a vector
            result = stmt->executeQuery("select * from stati");
            while(result->next())
                config.stati.push_back(new tasker::status(result->getString("name"), result->getInt("r"), result->getInt("g"), result->getInt("b")));
            
            delete result;

            // Create a map of the name of supertasks to their colors
            std::map<std::string, ImColor> colors;
            result = stmt->executeQuery("select * from tasks_meta");
            while(result->next())
                colors.insert(std::pair<std::string, ImColor>(result->getString("name"), ImColor{result->getInt("r"), result->getInt("g"), result->getInt("b"), 225}));
            delete result;
            
            // Construct the supertask objects, and assign their colors from the map
            result = stmt->executeQuery("show tables");
            while(result->next()) {
                std::string table = result->getString("Tables_in_" + connection->getSchema());

                // Substring out the "task_" prefix
                if(table.substr(0, 5) != "task_") continue;

                // Create a display name by substituting the "_" characters for spaces
                std::string display = table.substr(5);
                for(int i = 0; i < display.length(); i++) if(display[i] == '_') display[i] = ' ';
                tasker::supertask* task = new tasker::supertask(table.substr(5), display, colors.at(table.substr(5)));
                
                // Iterate through all tasks of supertask and load them into the object
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

    // Update a specific task on the server. Updates the task as a whole instead of individual parts for ease of code
    return_code update_task(const std::string& table, task* t) {
        if(has_open_connection() != tasker::return_code::True || connection->getSchema() == "") return tasker::return_code::Error;
        sql::PreparedStatement* stmt = nullptr;

        try {
            // Prepareed statement wont take parameter as a table name, so concatenation is required for the supertask name
            stmt = connection->prepareStatement("update task_" + table + " set task=?, status=?, people=?, date=?, pos=? where idd=?");

            // Set the actual values
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

    // Create a new supertask
    return_code create_category(const std::string& _name, float* colors) {
        if(has_open_connection() != tasker::return_code::True || connection->getSchema() == "") return tasker::return_code::Error;
        sql::PreparedStatement* stmt = nullptr;
        std::string name = _name;
        for(int i = 0; i < name.length(); i++) if(isspace(name[i])) name[i] = '_';

        try {
            // Create the table representation of the supertask
            stmt = connection->prepareStatement("create table task_" + name + "(task varchar(256), status varchar(64), people varchar(256), date varchar(16), pos int DEFAULT 0, idd int NOT NULL AUTO_INCREMENT, primary key(idd))");
            stmt->executeUpdate();
            delete stmt;

            // Add the supertask to the metadata table to store its color
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

    // Completely removes a supertask
    return_code drop_category(const std::string& category) {
        if(has_open_connection() != tasker::return_code::True || connection->getSchema() == "") return tasker::return_code::Error;
        sql::Statement* stmt = nullptr;

        try {
            stmt = connection->createStatement();

            // Drop the table itself
            stmt->executeUpdate("drop table task_" + category);
            
            // Remove the supertask from the metadata table
            stmt->executeUpdate("delete from tasks_meta where name=\"" + category + '"');
            delete stmt;
        } catch(sql::SQLException& e) {
            std::cout << "Error deleting category!" << std::endl << e.what() << std::endl;
            delete stmt;
            return return_code::Error;
        }

        return return_code::True;
    }

    // Remove a status soption
    return_code remove_status(const status* s, workspace& workspace) {
        if(has_open_connection() != tasker::return_code::True || connection->getSchema() == "") return tasker::return_code::Error;
        sql::Statement* stmt = nullptr;

        try {
            stmt = connection->createStatement();
            stmt->executeUpdate("delete from stati where name=\"" + std::string(s->name) + "\"");

            // Make sure to iterate through all supertasks in the workspace to change all the stati that used to be the deleted option to "None"
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

    // Create a new status
    return_code create_status(const std::string& name, const int& r, const int& g, const int& b) {
        if(has_open_connection() != tasker::return_code::True || connection->getSchema() == "") return tasker::return_code::Error;
        sql::PreparedStatement* stmt = nullptr;

        try {
            stmt = connection->prepareStatement("insert into stati(name, r, g, b) values (?, ?, ?, ?)");
            stmt->setString(1, name);
            stmt->setInt(2, r);
            stmt->setInt(3, g);
            stmt->setInt(4, b);
            stmt->executeUpdate();
            delete stmt;
        } catch(sql::SQLException& e) {
            std::cout << "Error while creating status!" << std::endl << e.what() << std::endl;
            delete stmt;
            return return_code::Error;
        }
        return return_code::True;
    }

    // Delete task from a supertask. Use the id to delete, rather than pass the whole task as a parameter
    return_code delete_task(const int& id, const std::string& super) {
        if(has_open_connection() != tasker::return_code::True || connection->getSchema() == "") return tasker::return_code::Error;
        sql::PreparedStatement* stmt = nullptr;

        try {
            stmt = connection->prepareStatement("delete from task_" + super + " where idd=?");
            stmt->setInt(1, id);
            stmt->executeUpdate();
        } catch(sql::SQLException& e) {
            std::cout << "Error while deleting task " << id << std::endl << e.what() << std::endl;
            delete stmt;
            return return_code::Error;
        }

        return return_code::True;
    }

    // Creates new task
    return_code create_task(const task* task, const std::string& table) {
        if(has_open_connection() != tasker::return_code::True || connection->getSchema() == "") return tasker::return_code::Error;
        sql::PreparedStatement* stmt = nullptr;

        try {
            stmt = connection->prepareStatement("insert into task_" + table + "(task, status, people, date, pos) values(?, ?, ?, ?, ?)");
            stmt->setString(1, task->taskk);
            stmt->setString(2, task->statuss->name);
            stmt->setString(3, task->people);
            stmt->setString(4, task->date);
            stmt->setInt(5, task->pos);
            stmt->executeUpdate();
            delete stmt;
        } catch(sql::SQLException& e) {
            std::cout << "Error while creating task!" << std::endl << e.what() << std::endl;
            delete stmt; 
            return return_code::Error;
        }

        return return_code::True;
    }

    // Constructors for structs
    status::status(const std::string& _name, const int& r, const int& g, const int& b) {
        std::copy(&_name[0], &_name[_name.length()], name);
        color = ImColor(r, g, b, 150);
    }

    task::task(status* _statuss, const std::string& _taskk, const std::string& _date, const std::string& _people, const int& _pos, const int& _id) {
        statuss = _statuss;
        pos = _pos;
        id = _id;
        std::copy(&_taskk[0], &_taskk[_taskk.length()], taskk);
        std::copy(&_date[0], &_date[_date.length()], date);
        std::copy(&_people[0], &_people[_people.length()], people);
    }

    supertask::supertask(const std::string& _name, const std::string& _display_name, const ImColor& _color) {
        std::copy(&_name[0], &_name[_name.length()], name);
        std::copy(&_display_name[0], &_display_name[_display_name.length()], display_name);
        color = _color;
        newTask = new task(nullptr, "\0", "\0", "\0", 0, 0);
    }

    workspace::workspace(const std::string& _name) {
        std::copy(&_name[0], &_name[_name.length()], name);
    }

    workspace::~workspace() {
        for(status* s : stati) delete s;
        for(supertask* t : tasks) delete t;
    }

    status* workspace::get_status(const std::string& name) {
        status* toreturn;
        for(status* s : stati) if(std::string(s->name) == name) return s;
        return nullptr;
    }
}