#include <cppconn/connection.h>

// Defines for creating the StringTooLongException error message at compile time
#define _STR(X) #X
#define STR(X) _STR(X)

#include "imgui.h"
#include "structs.h"
#include "server_queries.h"
using std::string;

namespace tasker {
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

    void queueQuery(ServerRequest* request) {
       requestQueue.access()->push(request);  
       requestQueue.release();
    }

    void serverRequestDispatcher() {
        while(true) {
            ServerRequest* request = nullptr;
            if(requestQueue.access()->size() != 0) {
                request = requestQueue.access()->front();
                requestQueue.access()->pop();
            }
            requestQueue.release();
            if(request != nullptr) request->execute();
            delete request;
        }
    }

}
