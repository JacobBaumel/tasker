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

        public:
            mutex_resource(T* data);
            ~mutex_resource();
            T* access();
            void release();
    };

    class status {
        friend class workspace;
        private:
            string* name;
            ImVec4* color;

            status(const std::string& _name, const int r, const int g, const int b);
            ~status();
    };

    class task {
        friend class workspace;
        friend class supertask;
        private:
            status* statuss;
            string* taskk;
            string* date;
            string* people;
            int* pos;
            int* id;

            task(status* _statuss, const std::string& _taskk, const std::string& _date, const std::string& _people, const int& _pos, const int& _id);
            ~task();
    };

    class supertask {
        friend class workspace;
        private:
            std::vector<task*>* tasks;
            ImVec4* color;
            string* name;
            string* display_name;

            supertask(const std::string& _name, const std::string& _display_name, const ImVec4& _color);
            ~supertask();
    };

    class workspace {
        private:
            string name;
            std::thread* requestThread;
            mutex_resource<sql::Connection>* connection;
            mutex_resource<std::vector<status*>>* stati;
            mutex_resource<std::vector<supertask*>>* tasks;
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

            void createCategory(const string& name, const ImVec4& color);
            void dropCategory(supertask* s);
            void setCategoryColor(supertask* s, const ImVec4& color);
            void setCategoryName(supertask* s, const string& name);
            string getCategoryName(supertask* s);
            ImVec4 getCategoryColor(supertask* s);
            task* getTasks();
            int numTasks();

            void createTask(supertask* super);
            void dropTask(supertask* super, task* t);
            char* getTaskString();
            char* getDateString();
            char* getAssigneesString();

            void createStatus(const string& text, const ImVec4& color);
            void dropStatus(status* s);
            void setStatusText(status* s, const string& text);
            string getStatusText(status* s);
            ImVec4& getStatusColor(status* s);
            status* getStatusFromString(const string& text);

    };
}

#endif
