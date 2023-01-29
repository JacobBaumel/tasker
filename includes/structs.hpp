#ifndef STRUCTS_H
#define STRUCTS_H
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
    class StringTooLongException : public std::exception {
        public:
            const char* what();
    };

    class TaskerException : public std::exception {
        private:
            const char* text;

        public:
            const char* what();
            TaskerException(const char* message);
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
            void connect();

            supertask* createCategory(const string& name, const ImVec4& color);
            void dropCategory(supertask* s);
            void setCategoryColor(supertask* s, const ImVec4& color);
            void setCategoryName(supertask* s, const string& name);

            void setTaskStatus(task* task, status* status);
            void setTaskDate(task* task, const string& date);
            void setTaskPeople(task* task, const string& date);
            void setTaskTask(task* task, const string& people);
            

            status* getStatusFromString(const string&);
#ifdef TASKER_DEBUG
            string toString();
#endif

    };
}

#endif
