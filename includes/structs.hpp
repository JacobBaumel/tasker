#ifndef STRUCTS_H
#define STRUCTS_H
#include "jsonstuff.h"
#define MAX_STRING_LENGTH 256

#include "imgui.h"
#include <cppconn/connection.h>
#include <string>
#include <vector>
#include <mutex>
#include <queue>
#include <thread>
#include <string>

using std::string;

namespace tasker {
    sql::Connection* getConnection(json_sql_connection& c);

    class StringTooLongException : public std::exception {
        public:
            const char* what();
    };

    // code meanings:
    // 0: nothing
    // 1: cant connect
    // 2: something doesnt exist
    // 3: something already exists
    // 4: null/empty value
    class TaskerException : public std::exception {
        private:
            const char* text;

        public:
            const char* what();
            const int* code;
            TaskerException(const char* message, const int& code);
    };

    template<typename T>
    class mutex_resource {
        private:
            std::mutex m;
            T* resource;
            bool locked;
            bool deleteResource;

        public:
            mutex_resource(T* data, bool deleteResource);
            ~mutex_resource();
            T* access();
            void release();
    };

    class status {
        friend class workspace;
        private:
            string* name;
            ImVec4* color;

            status(const std::string& _name, const ImVec4& color);
            ~status();

        public:
            const ImVec4* getColor();
            const char* getName();
    };

    class supertask;

    class task {
        friend class workspace;
        friend class supertask;
        private:
            supertask* super;
            status* statuss;
            string* taskk;
            string* date;
            string* people;
            int* pos;
            int* id;

            task(supertask* super, status* _statuss, const std::string& _taskk, const std::string& _date, const std::string& _people, const int& _pos, const int& _id);
            ~task();

        public:
            const status* getStatus();
            const char* getTask();
            const char* getDate();
            const char* getPeople();
    };

    class supertask {
        friend class workspace;
        private:
            std::vector<task*>* tasks;
            ImVec4* color;
            string* name;
            string* display_name;

            supertask(const string& _name, const ImVec4& _color);
            supertask(const ImVec4& _color, const string& _display_name);
            ~supertask();

        public:
            const ImVec4* getColor();
            const char* getName();
            const char* getDisplay();
            const std::vector<task*>* getTasks();
    };

    class workspace {
        private:
            string name;
            std::thread* requestThread;
            mutex_resource<sql::Connection>* connection;
            std::vector<status*>* stati;
            std::vector<supertask*>* tasks;
            mutex_resource<bool>* stopThread;
            mutex_resource<std::queue<string>>* actionQueue;

            void queueQuery(string);
            void requestDispatcher();

            template<typename T>
            void clearDynamicMemoryVector(std::vector<T>* vector);

        public:
            workspace(sql::Connection* _connection, const string& _name);
            ~workspace();

            void fullRefresh();
            void pushData();
            void create();
            void connect(const bool& pulldata);

            supertask* createCategory(const string& name, const ImVec4& color);
            void dropCategory(supertask* s);
            void setCategoryColor(supertask* s, const ImVec4& color);
            void setCategoryName(supertask* s, const string& name);
            const std::vector<supertask*>* getSupers();

            void setTaskStatus(task* task, status* status);
            void setTaskDate(task* task, const string& date);
            void setTaskPeople(task* task, const string& date);
            void setTaskTask(task* task, const string& people);
            task* createTask(supertask* super, status* status, const string& task, const string& people, const string& date);
            void dropTask(task* task);

            const std::vector<status*>* getStati();
            status* createStatus(const string& name, const ImVec4& color);
            void dropStatus(status* statuss);
            void setStatusColor(status* status, const ImVec4& color);
            void setStatusName(status* status, const string& name);

            status* getStatusFromString(const string&);
#ifdef TASKER_DEBUG
            string toString();
#endif

    };
}

#endif
