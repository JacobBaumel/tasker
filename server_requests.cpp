#include <cppconn/connection.h>
#include <cppconn/statement.h>
#include <cppconn/prepared_statement.h>
#include <cppconn/resultset.h>
#include <cppconn/driver.h>
#include <mutex>
#include <sstream>
#include <string>
#include "cppconn/exception.h"
#include "jsonstuff.h"
#include "structs.hpp"

// Defines for creating the StringTooLongException error message at compile time
#define _STR(X) #X
#define STR(X) _STR(X)

#include "imgui.h"

using std::string;

namespace tasker {
    sql::Connection* getConnection(json_sql_connection& c) {
        return get_driver_instance()->connect("tcp://" + c.ip + ":" + std::to_string(c.port), c.username, c.password);
    }

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
    TaskerException::TaskerException(const char* message, const int& _code) {
        this->code = new int(_code);
        this->text = message;
    }

    const char* TaskerException::what() {
        return text;
    }

    status::status(const string& _name, const ImVec4& _color) {
        name = new string(_name);
        color = new ImVec4(_color);
    }

    status::~status() {
        delete name;
        delete color;
    }

    const ImVec4* status::getColor() {
        return color;
    }

    const char* status::getName() {
        return name->c_str();
    }

    // The following are constructors/destructors for the task class
    task::task(supertask* super, status* _status, const std::string& _task, const std::string& _date, const std::string& _people, const int& _pos, const int& _id) {
        // Constructor will throw an error if any of the string arguments are longer than what is defined by MAX_STRING_LENGTH in includes/structs.h
        if(_task.length() > MAX_STRING_LENGTH || _date.length() > MAX_STRING_LENGTH || 
                _people.length() > MAX_STRING_LENGTH) throw StringTooLongException();
        taskk = new string(_task);
        date = new string(_date);
        people = new string(_people);
        statuss = _status;
        id = new int(_id);
        pos = new int(_pos);
        this->super = super;
    }

    task::~task() {
        delete taskk;
        delete date;
        delete people;
        delete pos;
        delete id;
    }

    const status* task::getStatus() {
        return statuss;
    }

    const char* task::getTask() {
        return taskk->c_str();
    }

    const char* task::getDate() {
        return date->c_str();
    }

    const char* task::getPeople() {
        return people->c_str();
    }

    // The following are constructors/destructors for the supertask class
    // This constructor is for building a supertask from a server side name
    supertask::supertask(const string& _name, const ImVec4& _color) {
        // Constructor will throw an error if any of the string arguments are longer than what is defined by MAX_STRING_LENGTH in includes/structs.h
        if(_name.length() > MAX_STRING_LENGTH) 
                    throw StringTooLongException();
        tasks = new std::vector<task*>();
        color = new ImVec4(_color);
        name = new string(_name);
        display_name = new string(_name);
        for(size_t i = 0; i < display_name->length(); i++) if((*display_name)[i] == '_') (*display_name)[i] = ' ';
    }

    // This constructor is for building a supertask from a user inputted display name
    supertask::supertask(const ImVec4& _color, const string& _display_name) {
        if(_display_name.length() > MAX_STRING_LENGTH) throw StringTooLongException();
        tasks = new std::vector<task*>();
        color = new ImVec4(_color);
        display_name = new string(_display_name);
        name = new string(_display_name);
        for(size_t i = 0; i < name->length(); i++) if((*name)[i] == ' ') (*name)[i] = '_';
    }

    supertask::~supertask() {
        for(task* t : *tasks) delete t;
        delete tasks;
        delete color;
        delete name;
        delete display_name;
    }

    const ImVec4* supertask::getColor() {
        return color;
    }

    const char* supertask::getName() {
        return name->c_str();
    }

    const char* supertask::getDisplay() {
        return display_name->c_str();
    }

    const std::vector<task*>* supertask::getTasks() {
        return tasks;
    }

    // The following are methods for the workspace class:
    
    // Constructor
    workspace::workspace(sql::Connection* _connection, const string& _name) {
        // Constructor should be provided with an already opened connection, otherwise an error is thrown
        if(_connection == nullptr || _connection->isClosed()) throw TaskerException("Connection cannot be null!", 1);

        // A workspace must have a name, even if it is not represented by a server yet (aka created)
        if(_name.empty()) throw TaskerException("Name cannot be empty!", 4);

        // The connection is added to a mutex resource, to allow for threading
        connection = new mutex_resource<sql::Connection>(_connection, false);

        // Vectors of the stati and supertasks are created, and wrapped in a mutex_resource
        stati = new std::vector<status*>();
        tasks = new std::vector<supertask*>();
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
        for(status* s : *stati) delete s;
        delete stati;
        for(supertask* t : *tasks) delete t;
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
        clearDynamicMemoryVector(stati);
        while(result->next())
            stati->push_back(new tasker::status(result->getString("name"), ImVec4(result->getInt("r"), result->getInt("g"), result->getInt("b"), 255)));
        
        delete result;

        // Create a map of the name of supertasks to their colors
        std::map<string, ImColor> colors;
        result = stmt->executeQuery("select * from tasks_meta");
        while(result->next())
            colors.insert(std::pair<string, ImVec4>(result->getString("name"), ImVec4(result->getInt("r"), result->getInt("g"), result->getInt("b"), 255)));
        delete result;
        
        // Construct the supertask objects, and assign their colors from the map
        result = stmt->executeQuery("show tables");
        clearDynamicMemoryVector(tasks);
        while(result->next()) {
            std::string table = result->getString("Tables_in_" + connection->access()->getSchema());
            connection->release();

            // Substring out the "task_" prefix
            if(table.substr(0, 5) != "task_") continue;

            // Create a display name by substituting the "_" characters for spaces
            std::string display = table.substr(5);
            for(size_t i = 0; i < display.length(); i++) if(display[i] == '_') display[i] = ' ';
            tasker::supertask* task = new tasker::supertask(table.substr(5), colors.at(table.substr(5)));
            
            // Iterate through all tasks of supertask and load them into the object
            sql::ResultSet* tasks = stmt->executeQuery("select * from " + table + " order by pos");
            while(tasks->next()) 
                task->tasks->push_back(new tasker::task(task, getStatusFromString(tasks->getString("status")), tasks->getString("task"), 
                            tasks->getString("date"), tasks->getString("people"), tasks->getInt("pos"), tasks->getInt("idd")));
        
            this->tasks->push_back(task);
        }
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
        int size = stati->size();
        for(int i = 0; i < size; i++) {
            status* s = stati->at(i);
            stmt->setString(1, *s->name);
            int r = s->color->x;
            int g = s->color->y;
            int b = s->color->z;
            stmt->setInt(2, r);
            stmt->setInt(3, g);
            stmt->setInt(4, b);
            stmt->setInt(5, r);
            stmt->setInt(6, g);
            stmt->setInt(7, b);
            stmt->executeUpdate();
        }
        delete stmt;
        
        // Prepare statement to be used to insert/update tables into the tasks_meta table to keep track of color
        stmt = connection->access()->prepareStatement("INSERT INTO tasks_meta(name, r, g, b) "
                "VALUES (?, ?, ?, ?) ON DUPLICATE KEY UPDATE r=?, g=?, b=?");

        // General statement to allow for creating supertask tables if they do not already exist
        sql::Statement* sstmt = connection->access()->createStatement();
        connection->release();

        // Iterate through all supertasks
        for(supertask* s : *tasks) {
            // Create table if not exists
            sstmt->executeUpdate(string("CREATE TABLE IF NOT EXISTS task_").append(*s->name).append(
                        "(task varchar(256), status varchar(64), people varchar(256), date varchar(16),"
                        " pos int DEFAULT 0, idd int NOT NULL AUTO_INCREMENT, primary key(idd))"));

            // Set parameters for insertion into tasks_meta, then update server
            stmt->setString(1, string("task_").append(*s->name));
            int r = s->color->x;
            int g = s->color->y;
            int b = s->color->z;
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
            throw TaskerException("Workspace already exists!", 3);
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
    void workspace::connect(const bool& pulldata) {
        // Detect if database already exists
        sql::Statement* stmt = connection->access()->createStatement();
        connection->release();
        sql::ResultSet* r = stmt->executeQuery("show databases like \"" + name + "\"");
        int count = r->rowsCount();
        delete stmt;
        delete r;
        if(count == 0) throw TaskerException("Workspace supplied does not already exist!!", 2);

        if(!pulldata) return;

        // // Set new schema and full refresh the local workspace
        connection->access()->setSchema(name);
        connection->release();
        fullRefresh();
    }

    // Creates a new category based on the name and color provided
    supertask* workspace::createCategory(const string& name, const ImVec4& color) {
        // Search through existing supertasks and make sure that the name does not already exist
        for(supertask* t : *tasks) if(*t->display_name == name) throw TaskerException("Supertask already exists!", 2);

        // Create the supertask and add it to the list of workspace supertasks
        supertask* t = new supertask(color, name);
        tasks->push_back(t);

        // Create an sql query to update the server with the change, and allow the request dispatcher to handle it
        // Create the table itself
        std::ostringstream ss;
        ss << "CREATE TABLE task_" << *t->name << "(task varchar(256), status varchar(64), people varchar(256), date varchar(16),"
                        " pos int DEFAULT 0, idd int NOT NULL AUTO_INCREMENT, primary key(idd))";
        queueQuery(ss.str());
        ss.str("");

        // Add an entry to the tasks_meta table to store color
        ss << "INSERT INTO tasks_meta(name, r, g, b) VALUES (\"" << *t->name << "\", " << ((int) color.x) <<
            ", " << ((int) color.y) << ", " << ((int) color.z) << ")";
        queueQuery(ss.str());

        // Return the newly created task for the UI to use
        return t;
    }

    // Changes a supertasks color to something else
    void workspace::setCategoryColor(supertask* s, const ImVec4& color) {
        // Delete the old pointer, and assign a new one with changed values
        delete s->color;
        s->color = new ImVec4(color);

        // Update sql database
        std::ostringstream ss;
        ss << "UPDATE tasks_meta SET r=" << ((int) color.x) << ", g=" << ((int) color.y) << ", b=" << 
            ((int) color.z) << " WHERE name=\"" << *s->name << '"';
        queueQuery(ss.str());
    }

    // Changes a supertasks name to something else
    void workspace::setCategoryName(supertask* s, const string& name) {
        // Delete the old pointer, and assign a new one with the changed values
        // First change the display name
        delete s->display_name;
        s->display_name = new string(name);
        string nname = name;
        
        // Modify name to be suited as an sql table name
        for(size_t i = 0; i < nname.length(); i++) if(nname[i] == ' ') nname[i] = '_';

        // Change the name of the table itself
        std::ostringstream ss;
        ss << "ALTER TABLE task_" << *s->name << " RENAME TO task_" << nname;
        queueQuery(ss.str());
        ss.str("");

        // Change the tasks_meta entry to match the new name
        ss << "UPDATE tasks_meta SET name=\"" << nname << "\" WHERE name=\"" << *s->name << '"';
        queueQuery(ss.str());

        // Finally change the local server-version name to the new version
        delete s->name;
        s->name = new string(nname);
    }

    // Delete a supertask and its subtasks from the workspae
    void workspace::dropCategory(supertask* t) {
        // Ensure the supertask exists before proceeding
        int index = -1;
        for(size_t i = 0; i < tasks->size(); i++) if(tasks->at(i) == t) index = i;
        if(index == -1) throw TaskerException("Supertask doesn't exist!", 2);

        // Delete the supertask from the list
        tasks->erase(tasks->begin() + index);

        // Drop the sql table
        std::ostringstream ss;
        ss << "DROP TABLE task_" << *t->name;
        queueQuery(ss.str());
        ss.str("");

        // Delete the tasks_meta entry for the supertask
        ss << "DELETE FROM tasks_meta WHERE name=\"" << *t->name << '"';
        queueQuery(ss.str());

        // Delete the supertask from memory
        delete t;
    }

    // Returns a const vector of all the known supertasks, so the UI can iterate over them and access them.
    // The UI does still have to use the setter methods provided by the workspace object
    const std::vector<supertask*>* workspace::getSupers() {
        return tasks;
    }

    // Sets the status pointer of a task object
    void workspace::setTaskStatus(task* task, status* status) {
        // Set the pointer
        task->statuss = status;

        // Update sql server with new information
        std::ostringstream ss;
        ss << "UPDATE TABLE " << task->super->getName() << " SET status=\"" << status->name << "\" WHERE idd=" << task->id;
        queueQuery(ss.str());

    }

    // Sets the date field for a task
    void workspace::setTaskDate(task* task, const string& date) {
        // Deletes the old date and creates a new one
        delete task->date;
        task->date = new string(date);

        // Updates server with new information
        std::ostringstream ss;
        ss << "UPDATE TABLE " << task->super->getName() << " SET date=\"" << date << "\" WHERE idd=" << task->id;
        queueQuery(ss.str());
    }

    // Sets the people field for a task
    void workspace::setTaskPeople(task* task, const string& people) {
        // Delete the old people field and creates the new one
        delete task->people;
        task->people = new string(people);

        // Updates sql server with new information
        std::ostringstream ss;
        ss << "UPDATE TABLE " << task->super->getName() << " SET people=\"" << people << "\" WHERE idd=" << task->id;
        queueQuery(ss.str());
    }
    
    // Sets task field for a task
    void workspace::setTaskTask(task* task, const string& taskString) {
        // Delete the old task string
        delete task->taskk;
        task->taskk = new string(taskString);

        // Update server
        std::ostringstream ss;
        ss << "UPDATE TABLE " << task->super->getName() << "SET task=\"" << taskString << "\" WHERE idd=" << task->id;
        queueQuery(ss.str());
    }
    
    // Creates a new task in a supertask
    task* workspace::createTask(supertask* super, status* status, const string& taskk, const string& people, const string& date) {
        // Sets the tasks pos to be at the end of the list
        int pos = super->tasks->size();

        // Starts with an ID of 0, finds the largest ID, and goes one higher
        int id = 0;
        for(const tasker::task* t : *super->tasks) if(*t->id > id) id = *t->id;
        id++;

        // Creates the new task with the ID and position information
        tasker::task* newT = new tasker::task(super, status, taskk, date, people, pos, id);

        // Push pointer to vector
        super->tasks->push_back(newT);

        // Updates server with the change
        std::ostringstream ss;
        ss << "INSERT INTO task_" << super->getName() << " (task, status, people, date, pos, id) VALUES (\"" << newT->taskk <<
            "\", \"" << newT->statuss->name << "\", \"" << newT->people << "\", \"" << newT->date << "\", " << newT->pos << ", " << newT->id << ")";
        queueQuery(ss.str());

        // Returns task for use by the UI
        return newT;
    }

    // Deletes a task from a supertask
    void workspace::dropTask(task* task) {
        // Get index of the task
        size_t index;
        for(index = 0; task->id != task->super->tasks->at(index)->id; index++); 

        // Remove task from supertask vector
        task->super->tasks->erase(task->super->tasks->begin() + index);

        // Update server
        std::ostringstream ss;
        ss << "DELETE FROM task_" << task->super->getName() << "WHERE idd=" << task->id;
        queueQuery(ss.str());
        delete task;
    }

    // Returns const vector of all the available stati, so the UI can iterate over them
    const std::vector<status*>* workspace::getStati() {
        return stati;
    }

    // Creates a new status
    status* workspace::createStatus(const string& name, const ImVec4& color) {
        // Create the pointer
        status* newS = new status(name, color);

        // Add pointer to vector of available stati
        stati->push_back(newS);

        // Add new status to the stati table
        std::ostringstream ss;
        ss << "INSERT INTO stati (name, r, g, b) values (\"" << name << "\", " << (int) color.x << ", " << (int) color.y << ", " << (int) color.z << ")";
        queueQuery(ss.str());

        // Return new status for use
        return newS;
    }

    // Deletes status from the workspace
    void workspace::dropStatus(status* status) {
        size_t index;
        for(index = 0; status != stati->at(index); index++);
        stati->erase(stati->begin() + index);
        std::ostringstream ss;
        ss << "DELETE FROM stati WHERE name=\"" << status->name << "\"";
        queueQuery(ss.str());
        delete status;
    }

    // Changes status color
    void workspace::setStatusColor(status* status, const ImVec4& color) {
        // Delete the old color and create the new one
        delete status->color;
        status->color = new ImVec4(color);

        // Update server with new color
        std::ostringstream ss;
        ss << "UPDATE TABLE stati SET r=" << (int) color.x << ", g=" << (int) color.y << ", b=" << (int) color.z << " WHERE name=\"" << status->name << "\"";
        queueQuery(ss.str());
    }

    // Change the name of a status
    void workspace::setStatusName(status* status, const string& name) {
        // Update server first so the old name can be used in the query
        std::ostringstream ss;
        ss << "UPDATE TABLE stati SET name=\"" << name << "\" WHERE name=\"" << status->name << "\"";
        queueQuery(ss.str());

        // Delete old name and create new one
        delete status->name;
        status->name = new string(name);
    }

    // Function to add sql query to the queue, to be executed
    void workspace::queueQuery(string query) {
        actionQueue->access()->push(query);
        actionQueue->release();
    }

    // Method that actually executes sql queries, to keep load times away from main thread
    void workspace::requestDispatcher() {
        bool hasRequests = false;
        // Detect whether main thread has ordered this thread to stop
        while(!*stopThread->access() || hasRequests) {
            stopThread->release();
            hasRequests = false;

            // Get the latest query from the queue
            string query;
            if(actionQueue->access()->size() != 0) {
                hasRequests = true;
                query = actionQueue->access()->front();
                actionQueue->access()->pop();
            }
            actionQueue->release();

            // Create statement, and execute the query
            if(!query.empty()) {
                sql::Statement* stmt = connection->access()->createStatement();
                connection->release();
                std::cout << "Running query: " << query << std::endl;
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
        for(status* s : *stati) if(*s->name == text) {
            return s;
        }
        throw TaskerException("Status does not exist!!", 1);
    }

#ifdef TASKER_DEBUG
    string workspace::toString() {
        string space = "stati:\n";

        for(status* s : *stati) space.append(*s->name + ": color (" + std::to_string((int) s->color->x) + 
                " " + std::to_string((int) s->color->y) + " " + std::to_string((int) s->color->z) + " " + std::to_string((int) s->color->w) + ")\n");

        space.append("\n\nSupertasks:\n");
        for(supertask* s : *tasks) {
            space.append("name: " + *s->name + "\ndisplay name: " + *s->display_name + "\nColor: " + std::to_string((int) s->color->x) +
                " " + std::to_string((int) s->color->y) + " " + std::to_string((int) s->color->z) + " " + std::to_string((int) s->color->w) + "\nTasks:\n\n");
            for(task* t : *s->tasks) {
               space.append(std::to_string(*t->id) + ": " + *t->taskk + "\n");
            }

            space.append("\n\n");
        }


        return space;
    }
#endif
}
