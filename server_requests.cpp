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
    // The following are methods for the mutex_resource class:

    // Constructor:
    template<typename T>
    mutex_resource<T>::mutex_resource(T* t, bool _deleteResource) {
        resource = t;
        
        // Boolean to allow for control over deleting resource when the mutex_resource itself is deleted
        deleteResource = _deleteResource;

        // Locked is to keep track of whether the mutex has already been locked, to avoid conflicts
        // when calling "access" multiple times without releasing the lock, and vice versa with "release"
        locked = false;
    }

    // Method that waits for a lock on the resource mutex, then returns the pointer to the value
    // Ultimately, it is up to the programmer not to save the pointer and use it outside of the
    // mutex resource
    template<typename T>
    T* mutex_resource<T>::access() {
        if(!locked) {
            m.lock();
            locked = true;
        }

        return resource;
    }

    // Releases the lock to allow then next thread in line to acquire a lock
    template<typename T>
    void mutex_resource<T>::release() {
        if(locked) m.unlock();
        locked = false;
    }

    // Destructor
    template<typename T>
    mutex_resource<T>::~mutex_resource() {
        if(deleteResource) delete resource;
    }

    // Error to throw when strings for sql queries are too long. At the time of documenting,
    // this number is 256 characters for all strings, or as defined by MAX_STRING_LENGTH
    // in includes/structs.h
    const char* StringTooLongException::what() {
        return "String exceeds maximum acceptable string length (" 
            STR(MAX_STRING_LENGTH) ")";
    }

    // More general exception for other things besides string length
    // Constructor to allow for custom error message
    TaskerException::TaskerException(const char* message) {
        text = message;
    }

    const char* TaskerException::what() {
        return text;
    }

    // The following are constructors/destructors for the status class:
    status::status(const std::string& _name, const int r, const int g, const int b) {
        if(_name.length() > MAX_STRING_LENGTH) throw StringTooLongException();
        name = new string(_name);
        color = new ImVec4(r, g, b, 1.0f);
    }

    status::~status() {
        delete name;
        delete color;
    }

    // The following are constructors/destructors for the task class
    task::task(status* _status, const std::string& _task, const std::string& _date, const std::string& _people, const int& _pos, const int& _id) {
        // Constructor will throw an error if any of the string arguments are longer than what is defined by MAX_STRING_LENGTH in includes/structs.h
        if(_task.length() > MAX_STRING_LENGTH || _date.length() > MAX_STRING_LENGTH || 
                _people.length() > MAX_STRING_LENGTH) throw StringTooLongException();
        taskk = new string(_task);
        date = new string(_date);
        people = new string(_people);
        statuss = _status;
        id = new int(_id);
        pos = new int(_pos);
    }

    task::~task() {
        delete taskk;
        delete date;
        delete people;
        delete pos;
        delete id;
    }

    // The following are constructors/destructors for the supertask class
    supertask::supertask(const std::string& _name, const std::string& _display_name, const ImVec4& _color) {
        // Constructor will throw an error if any of the string arguments are longer than what is defined by MAX_STRING_LENGTH in includes/structs.h
        if(_name.length() > MAX_STRING_LENGTH || _display_name.length() > MAX_STRING_LENGTH) 
                    throw StringTooLongException();
        name = new string(_name);
        display_name = new string(_display_name);
        tasks = new std::vector<task*>();
        color = new ImVec4(_color);
    }

    supertask::~supertask() {
        for(task* t : *tasks) delete t;
        delete tasks;
        delete color;
        delete name;
        delete display_name;
    }

    // The following are methods for the workspace class:
    
    // Constructor
    workspace::workspace(sql::Connection* _connection, const string& _name) {
        // Constructor should be provided with an already opened connection, otherwise an error is thrown
        if(_connection == nullptr || _connection->isClosed()) throw TaskerException("Connection cannot be null!");

        // A workspace must have a name, even if it is not represented by a server yet (aka created)
        if(_name.empty()) throw TaskerException("Name cannot be empty!");

        // The connection is added to a mutex resource, to allow for threading
        connection = new mutex_resource<sql::Connection>(_connection, false);

        // Vectors of the stati and supertasks are created, and wrapped in a mutex_resource
        stati = new mutex_resource<std::vector<status*>>(new std::vector<status*>(), true);
        tasks = new mutex_resource<std::vector<supertask*>>(new std::vector<supertask*>(), true);
        name = {_name};

        // stopThread is a control to end the query threads execution
        stopThread = new mutex_resource<bool>(new bool(false), true);
        
        // actionQueue is the queue used to keep track of the sql queries which must be executed
        actionQueue = new mutex_resource<std::queue<string>>(new std::queue<string>, true);
        
        // Starts the request dispatcher
        requestThread = new std::thread(&workspace::requestDispatcher, this);
    }

    workspace::~workspace() {
        // Sets the stopThread to true, which will stop the request dispatcher
        *stopThread->access() = true;
        stopThread->release();

        // Join the thread to wait for it to finish the queued items
        requestThread->join();

        // Close the connection and delete its mutex_resource
        connection->access()->close();
        delete connection;

        // Delete the remaining mutex_resources, and the contents of the vectors
        delete stopThread;
        delete actionQueue;
        delete requestThread; 
        for(status* s : *stati->access()) delete s;
        delete stati;
        for(supertask* t : *tasks->access()) delete t;
        delete tasks;
    }

    // Initiates an entire refresh of all data in the workspace, from the server
    void workspace::fullRefresh() {
        sql::Statement* stmt = nullptr;
        sql::ResultSet* result = nullptr;        
        
        // Create statement to be used throughout the fetching of data
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

    // Push all data from the local workspace to the sql server
    void workspace::pushData() {
        // Prepare statement to be used to insert new stati into the stati table, or update existing ones if already present
        sql::PreparedStatement* stmt = connection->access()->prepareStatement("INSERT INTO stati"
                "(name, r, g, b) values (?, ?, ?, ?) on duplicate key update r=?, g=?, b=?");
        connection->release();

        // Iterate through all stati, and send to server
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
        
        // Prepare statement to be used to insert/update tables into the tasks_meta table to keep track of color
        stmt = connection->access()->prepareStatement("INSERT INTO tasks_meta(name, r, g, b) "
                "VALUES (?, ?, ?, ?) ON DUPLICATE KEY UPDATE r=?, g=?, b=?");

        // General statement to allow for creating supertask tables if they do not already exist
        sql::Statement* sstmt = connection->access()->createStatement();
        connection->release();

        tasks->access();

        // Iterate through all supertasks
        for(supertask* s : *tasks->access()) {
            // Create table if not exists
            sstmt->executeUpdate(string("CREATE TABLE IF NOT EXISTS task_").append(*s->name).append(
                        "(task varchar(256), status varchar(64), people varchar(256), date varchar(16),"
                        " pos int DEFAULT 0, idd int NOT NULL AUTO_INCREMENT, primary key(idd))"));

            // Set parameters for insertion into tasks_meta, then update server
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

            // Statement for inserting/updating tasks in supertask table
            sql::PreparedStatement* tsend = connection->access()->prepareStatement(string("insert into task_")
                    .append(*s->name).append("(task, status, people, date, pos, idd) values(?, ?, ?, ?, ?, ?) "
                        "on duplicate key update task=?, status=?, people=?, date=?, pos=?"));
            connection->release();

            // Iterate through individual tasks
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
        tasks->release();
        delete stmt;
        delete sstmt;
    }

    // Create workspace database and prepare it for data
    void workspace::create() {
        sql::Statement* stmt = nullptr;
        sql::ResultSet* r = nullptr;

        stmt = connection->access()->createStatement();
        
        // Determin if the database already exists, and if it does, throw an error
        r = stmt->executeQuery("show databases like '" + connection->access()->getSchema() + "'");
        connection->release();
        if(r->rowsCount() > 0) {
            delete stmt;
            delete r;
            throw TaskerException("Workspace already exists!");
        }

        // Create the database, set the connection schema, and refresh the statement
        stmt->executeUpdate("create database " + name);
        connection->access()->setSchema(name);
        delete stmt;
        stmt = connection->access()->createStatement();
        connection->release();

        // Create the necessary tables including tasks_meta and stati, and insert the default status (None) into the stati table
        stmt->executeUpdate("create table tasks_meta (name varchar(64), r int, g int, b int, UNIQUE(name))");
        stmt->executeUpdate("create table stati (name varchar(64), r int, g int, b int, UNIQUE(name))");
        stmt->executeUpdate("insert into stati(name, r, g, b) values (\"None\", 0, 0, 0)");
        delete stmt;
    }

    // Method to change the connection schema and download data if the workspace is already created
    void workspace::connect() {
        // Detect if database already exists
        sql::Statement* stmt = connection->access()->createStatement();
        connection->release();
        sql::ResultSet* r = stmt->executeQuery("show databases like \"" + name + "\"");
        int count = r->rowsCount();
        delete stmt;
        delete r;
        if(count == 0) throw TaskerException("Workspace supplied does not already exist!!");
        

        // // Set new schema and full refresh the local workspace
        connection->access()->setSchema(name);
        connection->release();
        fullRefresh();
    }

    // Function to add sql query to the queue, to be executed
    void workspace::queueQuery(string query) {
        actionQueue->access()->push(query);
        actionQueue->release();
    }

    // Method that actually executes sql queries, to keep load times away from main thread
    void workspace::requestDispatcher() {
        // Detect whether main thread has ordered this thread to stop
        while(!*stopThread->access()) {
            stopThread->release();

            // Get the latest queery from the queue
            string query;
            if(actionQueue->access()->size() != 0) {
               query = actionQueue->access()->front();
               actionQueue->access()->pop();
            }
            actionQueue->release();

            // Create statement, and execute the query
            if(!query.empty()) {
                sql::Statement* stmt = connection->access()->createStatement();
                connection->release();
                stmt->executeUpdate(query);
                delete stmt;
            }
        }

        stopThread->release();
    }

    // Helper method to clear a vector of dynamically allocated data properly
    template<typename T>
    void workspace::clearDynamicMemoryVector(std::vector<T>* vector) {
        for(T t : *vector) delete t;
        vector->clear();
    }

    // Helper method to get a status pointer from its name
    status* workspace::getStatusFromString(const string& text) {
        for(status* s : *stati->access()) if(*s->name == text) {
            stati->release();
            return s;
        }

        stati->release();
        return nullptr;
    }

#ifdef TASKER_DEBUG
    string workspace::toString() {
        string space = "stati:\n";

        for(status* s : *stati->access()) space.append(*s->name + ": color (" + std::to_string((int) (s->color->w * 255)) + 
                " " + std::to_string((int) (s->color->x * 255)) + " " + std::to_string((int) (s->color->y * 255)) + ")\n");
        stati->release();

        space.append("\n\nSupertasks:\n");
        for(supertask* s : *tasks->access()) {
            space.append("name: " + *s->name + "\ndisplay name: " + *s->display_name + "\nColor: " + std::to_string((int) (s->color->w * 255)) +
                " " + std::to_string((int) (s->color->x * 255)) + " " + std::to_string((int) (s->color->y * 255)) + "\nTasks:\n\n");
            for(task* t : *s->tasks) {
               space.append(std::to_string(*t->id) + ": " + *t->taskk + "\n");
            }

            space.append("\n\n");
        }


        return space;
    }
#endif
    
}
