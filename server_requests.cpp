#include <cppconn/connection.h>
#include <cppconn/driver.h>
#include <mutex>

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

    status::status(const std::string& _name, const int r, const int g, const int b) {
        if(_name.length() > MAX_STRING_LENGTH) throw StringTooLongException();
        name = new char[MAX_STRING_LENGTH];
        std::copy(&_name[0], &_name[_name.length()], name);
        color = new ImVec4(r, g, b, 1.0f);
    }

    status::~status() {
        delete[] name;
        delete color;
    }

    task::task(status* _status, const std::string& _task, const std::string& _date, const std::string& _people, const int& _pos, const int& _id) {
        if(_task.length() > MAX_STRING_LENGTH || _date.length() > MAX_STRING_LENGTH || 
                _people.length() > MAX_STRING_LENGTH) throw StringTooLongException();
        taskk = new char[MAX_STRING_LENGTH];
        date = new char[MAX_STRING_LENGTH];
        people = new char[MAX_STRING_LENGTH];
        std::copy(&_task[0], &_task[_task.length()], taskk);
        std::copy(&_date[0], &_date[_date.length()], date);
        std::copy(&_people[0], &_people[_people.length()], people);
        statuss = _status;
        *id = {_id};
        *pos = {_pos};
    }

    task::~task() {
        delete[] taskk;
        delete[] date;
        delete[] people;
        delete pos;
        delete id;
    }

    supertask::supertask(const std::string& _name, const std::string& _display_name, const ImVec4& _color) {
        if(_name.length() > MAX_STRING_LENGTH || 
                _display_name.length() > MAX_STRING_LENGTH) 
                    throw StringTooLongException();
        name = new char[MAX_STRING_LENGTH];
        display_name = new char[MAX_STRING_LENGTH];
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
       connection = _connection;
       stati = new std::vector<status*>();
       tasks = new std::vector<supertask*>();
       name = {_name};
       stopThread = new mutex_resource<bool>(new bool(false));
       actionQueue = new mutex_resource<std::queue<func>>(new std::queue<func>);
       requestThread = new std::thread(&workspace::requestDispatcher, this);
    }

    workspace::~workspace() {
        *stopThread->access() = true;
        stopThread->release();
        requestThread->join();
        delete stopThread;
        delete actionQueue;
        delete requestThread;
        for(status* s : *stati) delete s;
        delete stati;
        for(supertask* t : *tasks) delete t;
        delete tasks;
    }

    void workspace::queueQuery(func f) {
        actionQueue->access()->push(f);
        actionQueue->release();
    }

    void workspace::requestDispatcher() {
        while(true) {
            func f = nullptr;
            if(actionQueue->access()->size() != 0) {
               f = actionQueue->access()->front();
               actionQueue->access()->pop();
            }
            actionQueue->release();
            if(f != nullptr) f();
            if(!*stopThread->access()) {
                stopThread->release();
                break;
            }
        }
    }
    
//    void queueQuery(ServerRequest* request) {
//       requestQueue.access()->push(request);  
//       requestQueue.release();
//    }
//
//    void serverRequestDispatcher() {
//        while(true) {
//            ServerRequest* request = nullptr;
//            if(requestQueue.access()->size() != 0) {
//                request = requestQueue.access()->front();
//                requestQueue.access()->pop();
//            }
//            requestQueue.release();
//            if(request != nullptr) request->execute();
//            delete request;
//        }
//    }

}
