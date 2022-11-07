#ifndef STRUCTS_H
#define STRUCTS_H
#define MAX_STRING_LENGTH 256

#include "imgui.h"
#include <cppconn/connection.h>
#include <string>
#include <vector>
#include <mutex>

using std::string;

namespace tasker {
    class StringTooLongException : public std::exception {
        public:
            const char* what();
    };

    template<typename T>
    class mutex_resource {
        private:
            T* data;
            std::mutex lock;
            bool locked;

        public:
            mutex_resource(T* data);
            T* access();
            void release();
    };

    class status {
        friend class workspace;
        private:
            char* name;
            ImVec4* color;

            status(const std::string& _name, const int r, const int g, const int b);
            ~status();
    };

    class task {
        friend class workspace;
        friend class supertask;
        private:
            status* statuss;
            char* taskk;
            char* date;
            char* people;
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
            char* name;
            char* display_name;

            supertask(const std::string& _name, const std::string& _display_name, const ImVec4& _color);
            ~supertask();
    };

    class workspace {
        private:
            sql::Connection* connection;
            std::vector<status*> stati;
            std::vector<supertask*> tasks;
            string name;
            status* getStatusPointer(const string& name);

        public:
            static constexpr int STRING_FIELD_LENGTH = MAX_STRING_LENGTH;

            workspace(const string& ip, const int& port, const string& user, const string& password);
            ~workspace();

            void fullRefresh();
            void create(const string& name);

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
