#include <cppconn/connection.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/driver.h>
#include <mutex>
#include "structs.h"

// Defines for creating the StringTooLongException error message at compile time
#define _STR(X) #X
#define STR(X) _STR(X)

#include "imgui.h"
#include "structs.h"
using std::string;

namespace tasker {
    template<typename T>
    mutex_resource<T>::mutex_resource(T* t) {
        resource = t;
        locked = false;
    }

    template<typename T>
    T* mutex_resource<T>::access() {
        if(!locked) {
            m.lock();
            locked = true;
        }

        return resource;
    }

    template<typename T>
    void mutex_resource<T>::release() {
        if(locked) m.unlock();
        locked = false;
    }

    template<typename T>
    mutex_resource<T>::~mutex_resource() {
        delete resource;
    }

    const char* StringTooLongException::what() {
        return "String exceeds maximum acceptable string length (" 
            STR(MAX_STRING_LENGTH) ")";
    }

    TaskerException::TaskerException(const char* message) {
        text = message;
    }
    const char* TaskerException::what() {
        return text;
    }

    status::status(const std::string& _name, const int r, const int g, const int b) {
        if(_name.length() > MAX_STRING_LENGTH) throw StringTooLongException();
        *name = _name;
        color = new ImVec4(r, g, b, 1.0f);
    }

    status::~status() {
        delete name;
        delete color;
    }

    task::task(status* _status, const std::string& _task, const std::string& _date, const std::string& _people, const int& _pos, const int& _id) {
        if(_task.length() > MAX_STRING_LENGTH || _date.length() > MAX_STRING_LENGTH || 
                _people.length() > MAX_STRING_LENGTH) throw StringTooLongException();
        *taskk = _task;
        *date = _date;
        *people = _people;
        statuss = _status;
        *id = {_id};
        *pos = {_pos};
    }

    task::~task() {
        delete taskk;
        delete date;
        delete people;
        delete pos;
        delete id;
    }

    supertask::supertask(const std::string& _name, const std::string& _display_name, const ImVec4& _color) {
        if(_name.length() > MAX_STRING_LENGTH || _display_name.length() > MAX_STRING_LENGTH) 
                    throw StringTooLongException();
        *name = _name;
        *display_name = _display_name;
        tasks = new std::vector<task*>();
        std::copy(&_name[0], &_name[_name.length()], name);
        std::copy(&_display_name[0], &_display_name[_display_name.length()], 
                display_name);
        color = new ImVec4(_color);
    }

    supertask::~supertask() {
        for(task* t : *tasks) delete t;
        delete tasks;
        delete color;
        delete[] name;
        delete[] display_name;
    }

    workspace::workspace(sql::Connection* _connection, const string& _name) {
        if(_connection == nullptr || _connection->isClosed()) throw TaskerException("Connection cannot be null!");
        if(_name.empty()) throw TaskerException("Name cannot be empty!");
        connection = new mutex_resource<sql::Connection>(_connection);
        stati = new mutex_resource<std::vector<status*>>(new std::vector<status*>());
        tasks = new mutex_resource<std::vector<supertask*>>(new std::vector<supertask*>());
        name = {_name};
        stopThread = new mutex_resource<bool>(new bool(false));
        actionQueue = new mutex_resource<std::queue<string>>(new std::queue<string>);
        requestThread = new std::thread(&workspace::requestDispatcher, this);
    }

    workspace::~workspace() {
        *stopThread->access() = true;
        stopThread->release();
        requestThread->join();
        connection->access()->close();
        delete stopThread;
        delete actionQueue;
        delete requestThread; 
        for(status* s : *stati->access()) delete s;
        delete stati;
        for(supertask* t : *tasks->access()) delete t;
        delete tasks;
    }

    void workspace::fullRefresh() {
        sql::Statement* stmt = nullptr;
        sql::ResultSet* result = nullptr;        
        
        // Create statement to bbe used throughout the fetching of data
        stmt = connection->access()->createStatement();
        connection->release();

        // Load status options into a vector
        result = stmt->executeQuery("select * from stati");
        clearDynamicMemoryVector(stati->access());
        while(result->next())
            stati->access()->push_back(new tasker::status(result->getString("name"), result->getInt("r"), result->getInt("g"), result->getInt("b")));
        stati->release();
        
        delete result;

        // Create a map of the name of supertasks to their colors
        std::map<string, ImColor> colors;
        result = stmt->executeQuery("select * from tasks_meta");
        while(result->next())
            colors.insert(std::pair<string, ImColor>(result->getString("name"), ImColor{result->getInt("r"), result->getInt("g"), result->getInt("b"), 225}));
        delete result;
        
        // Construct the supertask objects, and assign their colors from the map
        result = stmt->executeQuery("show tables");
        clearDynamicMemoryVector(tasks->access());
        while(result->next()) {
            std::string table = result->getString("Tables_in_" + connection->access()->getSchema());
            connection->release();

            // Substring out the "task_" prefix
            if(table.substr(0, 5) != "task_") continue;

            // Create a display name by substituting the "_" characters for spaces
            std::string display = table.substr(5);
            for(int i = 0; i < display.length(); i++) if(display[i] == '_') display[i] = ' ';
            tasker::supertask* task = new tasker::supertask(table.substr(5), display, colors.at(table.substr(5)));
            
            // Iterate through all tasks of supertask and load them into the object
            sql::ResultSet* tasks = stmt->executeQuery("select * from " + table + " order by pos");
            while(tasks->next()) 
                task->tasks->push_back(new tasker::task(getStatusFromString(tasks->getString("status")), tasks->getString("task"), tasks->getString("date"), tasks->getString("people"), tasks->getInt("pos"), tasks->getInt("idd")));
        
            this->tasks->access()->push_back(task);
        }
        this->tasks->release();
        delete stmt;
        delete result;
    }

    void workspace::pushData() {
        sql::PreparedStatement* stmt = connection->access()->prepareStatement("INSERT INTO stati"
                "(name, r, g, b) values (?, ?, ?, ?) on duplicate key update r=?, g=?, b=?");
        connection->release();
        int size = stati->access()->size();
        for(int i = 0; i < size; i++) {
            status* s = stati->access()->at(i);
            stmt->setString(1, *s->name);
            stmt->setInt(2, (int) (s->color->w * 255));
            stmt->setInt(3, (int) (s->color->x * 255));
            stmt->setInt(4, (int) (s->color->y * 255));
            stmt->executeUpdate();
        }
        stati->release();
        delete stmt;
        
        stmt = connection->access()->prepareStatement("INSERT INTO tasks_meta(name, r, g, b) "
                "VALUES (?, ?, ?, ?) ON DUPLICATE KEY UPDATE r=?, g=?, b=?");
        sql::Statement* sstmt = connection->access()->createStatement();
        connection->release();

        tasks->access();
        for(supertask* s : *tasks->access()) {
            sstmt->executeUpdate(string("CREATE TABLE IF NOT EXISTS task_").append(*s->name).append(
                        "(task varchar(256), status varchar(64), people varchar(256), date varchar(16),"
                        " pos int DEFAULT 0, idd int NOT NULL AUTO_INCREMENT, primary key(idd))"));
            stmt->setString(1, string("task_").append(*s->name));
            int r = (int) (s->color->w * 255);
            int g = (int) (s->color->x * 255);
            int b = (int) (s->color->y * 255);
            stmt->setInt(2, r);
            stmt->setInt(3, g);
            stmt->setInt(4, b);
            stmt->setInt(5, r);
            stmt->setInt(6, g);
            stmt->setInt(7, b);
            stmt->executeUpdate();

            sql::PreparedStatement* tsend = connection->access()->prepareStatement(string("insert into task_")
                    .append(*s->name).append("(task, status, people, date, pos, idd) values(?, ?, ?, ?, ?, ?) "
                        "on duplicate key update task=?, status=?, people=?, date=?, pos=?"));
            connection->release();

            for(task* t : *s->tasks) {
                tsend->setString(1, *t->taskk); 
                tsend->setString(2, *t->statuss->name);
                tsend->setString(3, *t->people);
                tsend->setString(4, *t->date);
                tsend->setInt(5, *t->pos);
                tsend->setInt(6, *t->id);
                tsend->setString(7, *t->taskk);
                tsend->setString(8, *t->statuss->name);
                tsend->setString(9, *t->people);
                tsend->setString(10, *t->date);
                tsend->setInt(11, *t->pos);
                tsend->executeUpdate();
            }

            delete tsend;
        }
        delete stmt;
        delete sstmt;
    }

    void workspace::create() {
        sql::Statement* stmt = nullptr;
        sql::ResultSet* r = nullptr;

        stmt = connection->access()->createStatement();
        
        r = stmt->executeQuery("show databases like '" + connection->access()->getSchema() + "'");
        connection->release();
        if(r->rowsCount() > 0) {
            delete stmt;
            delete r;
            throw TaskerException("Workspace already exists!");
        }

        stmt->executeUpdate("create database " + name);
        connection->access()->setSchema(name);
        delete stmt;
        stmt = connection->access()->createStatement();
        connection->release();
        stmt->executeUpdate("create table tasks_meta (name varchar(64), r int, g int, b int, UNIQUE(name))");
        stmt->executeUpdate("create table stati (name varchar(64), r int, g int, b int, UNIQUE(name))");
        stmt->executeUpdate("insert into stati(name, r, g, b) values (\"None\", 0, 0, 0)");
        delete stmt;
    }

    void workspace::queueQuery(string query) {
        actionQueue->access()->push(query);
        actionQueue->release();
    }

    void workspace::requestDispatcher() {
        while(true) {
            string query;
            if(actionQueue->access()->size() != 0) {
               query = actionQueue->access()->front();
               actionQueue->access()->pop();
            }
            actionQueue->release();
            if(!query.empty()) {
                sql::Statement* stmt = connection->access()->createStatement();
                connection->release();
                stmt->executeUpdate(query);
                delete stmt;
            }
            if(!*stopThread->access()) {
                stopThread->release();
                break;
            }
        }
    }

    template<typename T>
    void workspace::clearDynamicMemoryVector(std::vector<T>* vector) {
        for(T t : *vector) delete t;
        vector->clear();
    }
    
}
