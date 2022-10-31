#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/connection.h>
#include "structs.h"
#include "server_queries.h"
#include "jsonstuff.h"
#include <map>
#include <memory>
using std::string;

namespace tasker {
    bool ServerRequest::is_valid_connection(sql::Connection* connection) {
        return connection != nullptr && !connection->isClosed();
    }

    class CreateWorkspace : public ServerRequest {
        private:
            const string name;
            sql::Connection* connection;
        
        public:
            CreateWorkspace(sql::Connection* _connection, const std::string& _name) : name(_name), connection(_connection) {}

            return_code execute() {

                if(!is_valid_connection(connection)) return tasker::return_code::Error;
                sql::Statement* stmt = nullptr;

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
    };

    class GetData : public ServerRequest {
        private:
            workspace* space;
            sql::Connection* connection;

        public:
            return_code execute() {
                if(!is_valid_connection(connection) || connection->getSchema() == "") return tasker::return_code::Error;
                sql::Statement* stmt = nullptr;
                sql::ResultSet* result = nullptr;        
        
                try {
                    // Construct the workspace struct wuth the selected schema name
                    space = new workspace(connection->getSchema());
                    stmt = connection->createStatement();

                    // Load status options into a vector
                    result = stmt->executeQuery("select * from stati");
                    while(result->next())
                        space->stati.push_back(new tasker::status(result->getString("name"), result->getInt("r"), result->getInt("g"), result->getInt("b")));
            
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
                            task->tasks.push_back(new tasker::task(space->get_status(tasks->getString("status")), tasks->getString("task"), tasks->getString("date"), tasks->getString("people"), tasks->getInt("pos"), tasks->getInt("idd")));
            
                        space->tasks.push_back(task);
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

            GetData(sql::Connection* _connection, workspace* _space) : space(_space), connection(_connection) {}
    };

    class UpdateTask : public ServerRequest {
        private:
            sql::Connection* connection;
            const std::string& table;
            const task* t;

        public:
            return_code execute() {
                if(!is_valid_connection(connection) || connection->getSchema() == "") return tasker::return_code::Error;
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
            UpdateTask(sql::Connection* _connection, const std::string& _table, task* _t) : connection(_connection), table(_table), t(_t) {}
    };

    class CreateCategory : public ServerRequest {
        private:
            sql::Connection* connection;
            string name;
            float* colors;

        public:
            return_code execute() {
                if(!is_valid_connection(connection) || connection->getSchema() == "") return tasker::return_code::Error;
                sql::PreparedStatement* stmt = nullptr;
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
            CreateCategory(sql::Connection* _connection, std::string _name, float* _colors) : connection(_connection), name(_name), colors(_colors) {}
    };

    class DropCategory : public ServerRequest {
        private:
            sql::Connection* connection;
            const string name;

        public:
            return_code execute() {
                if(!is_valid_connection(connection)|| connection->getSchema() == "") return tasker::return_code::Error;
                sql::Statement* stmt = nullptr;

                try {
                    stmt = connection->createStatement();

                    // Drop the table itself
                    stmt->executeUpdate("drop table task_" + name);
            
                    // Remove the supertask from the metadata table
                    stmt->executeUpdate("delete from tasks_meta where name=\"" + name + '"');
                    delete stmt;
                } catch(sql::SQLException& e) {
                    std::cout << "Error deleting category!" << std::endl << e.what() << std::endl;
                    delete stmt;
                    return return_code::Error;
                }

                return return_code::True;
            }

            DropCategory(sql::Connection* _connection, const std::string& _name) : connection(_connection), name(_name) {}
    };

    class RemoveStatus : public ServerRequest {
        private:
            sql::Connection* connection;
            workspace* space;
            const status* s;

        public:
            return_code execute() {
                if(!is_valid_connection(connection) || connection->getSchema() == "") return tasker::return_code::Error;
                sql::Statement* stmt = nullptr;

                try {
                    stmt = connection->createStatement();
                    stmt->executeUpdate("delete from stati where name=\"" + std::string(s->name) + "\"");

                    // Make sure to iterate through all supertasks in the workspace to change all the stati that used to be the deleted option to "None"
                    for(supertask* tasks : space->tasks) {
                        for(task* t : tasks->tasks) {
                            if(t->statuss == s) {
                                t->statuss = space->get_status("None");
                                queueAction(new UpdateTask(connection, tasks->name, t));
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
            RemoveStatus(sql::Connection* _connection, const status* _s, workspace* _workspace) : connection(_connection), s(_s), space(_workspace) {}
    };

    class CreateStatus : public ServerRequest {
        private:
            sql::Connection* connection;
            const string name;
            const int r;
            const int g;         
            const int b;         

        public:
            return_code execute() {
                if(!is_valid_connection(connection) || connection->getSchema() == "") return tasker::return_code::Error;
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
            CreateStatus(sql::Connection* _connection, const string _name, const int& _r, const int& _g, const int& _b) : connection(_connection), 
                name(_name), r(_r), g(_g), b(_b) {}
    };

    class DeleteTask : public ServerRequest {
        private:
            sql::Connection* connection;
            const int id;
            const string super;

        public:
            return_code execute() {
                if(!is_valid_connection(connection) || connection->getSchema() == "") return tasker::return_code::Error;
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
            DeleteTask(sql::Connection* _connection, const int& _id, const std::string& _super) : connection(_connection), id(_id), super(_super) {}
    };

    class CreateTask : public ServerRequest {
        private:
            sql::Connection* connection;
            const task* t;
            const string table;

        public:
            return_code execute() {
                if(!is_valid_connection(connection) || connection->getSchema() == "") return tasker::return_code::Error;
                sql::PreparedStatement* stmt = nullptr;

                try {
                    stmt = connection->prepareStatement("insert into task_" + table + "(task, status, people, date, pos) values(?, ?, ?, ?, ?)");
                    stmt->setString(1, t->taskk);
                    stmt->setString(2, t->statuss->name);
                    stmt->setString(3, t->people);
                    stmt->setString(4, t->date);
                    stmt->setInt(5, t->pos);
                    stmt->executeUpdate();
                    delete stmt;
                } catch(sql::SQLException& e) {
                    std::cout << "Error while creating task!" << std::endl << e.what() << std::endl;
                    delete stmt; 
                    return return_code::Error;
                }

                return return_code::True;
            }
            CreateTask(sql::Connection* _connection, const task* _task, const std::string& _table) : connection(_connection), t(_task), table(_table) {}
    };

    void queueQuery(ServerRequest* request) {
        queueLock.lock();
        actionQueue->push(request);
        queueLock.unlock();
    }

    void serverRequestDispatcher() {
        while(true) {
            ServerRequest* request = nullptr;
            queueLock.lock();
            if(actionQueue->size() != 0) {
                request = actionQueue->front();
                actionQueue->pop();
            }
            queueLock.unlock();
            if(request != nullptr) request->execute();
            delete request;
        }
    }

}