#ifndef SERVER_QUERIES_H
#define SERVER_QUERIES_H

#include "structs.h"
#include "statement.h"
#include <string>
#include <mutex>
#include <queue>
using std::string;

namespace tasker {

    enum class return_code {
        Error = -1,
        False = 0,
        True = 1,
        None = 2,
    };

    class ServerRequest {
        protected:
            bool is_valid_connection(sql::Connection* connection);

        public:
            virtual return_code execute() = 0;
    };

    class CreateWorkspace : public ServerRequest {
        public:
            return_code execute();
            CreateWorkspace(sql::Connection* connection, const string& name);
    };

    class GetData : public ServerRequest {
        public:
            return_code execute();
            GetData(tasker::workspace* space);
    };

    class UpdateTask : public ServerRequest {
        public:
            return_code execute();
            UpdateTask(sql::Connection* connection, const string& table, task* t);
    };

    class CreateCategory : public ServerRequest {
        public:
            return_code execute();
            CreateCategory(sql::Connection* connection, const string& name, float* color);
    };

    class DropCategory : public ServerRequest {
        public:
            return_code execute();
            DropCategory(sql::Connection* connection, const string& name, float* color);
    };

    class RemoveStatus : public ServerRequest {
        public:
            return_code execute();
            RemoveStatus(sql::Connection* connection, const status* s, tasker::workspace* workspace);
    };

    class CreateStatus : public ServerRequest {
        public:
            return_code execute();
            CreateStatus(sql::Connection* connection, const string& name, const int& r, const int& g, const int& b);
    };

    class DeleteTask : public ServerRequest {
        public:
            return_code execute();
            DeleteTask(sql::Connection* connection, const int& id, const string& super);
    };

    class CreateTask : public ServerRequest {
        public:
            return_code execute();
            CreateTask(sql::Connection* connection, const task* task, const string& table);
    };

    extern std::mutex queueLock;
    extern std::queue<ServerRequest*>* actionQueue;
    
    void queueAction(ServerRequest* request);
    void serverRequestDispatcher();

}

#endif