#ifndef JSON_STUFF_H
#define JSON_STUFF_H

#include <iostream>
#include <vector>


namespace tasker {

    struct json_sql_connection {
        std::string ip;
        int port;
        std::string username;
        std::string password;

        bool operator==(const json_sql_connection& other) {
            return ip == other.ip && port == other.port && username == other.username && password == other.password;
        }
    };

    struct json_database {
        json_sql_connection connection;
        std::string schema;
    };

    struct database_array {
        std::vector<json_database> databases;
        std::vector<json_sql_connection> connections;
        json_database& get_database(int index) {return databases.at(index);}
        json_sql_connection& get_connection(int index) {return connections.at(index);}
    };

    int add_json_connection(const json_sql_connection& conn);
    void add_json_database(const tasker::json_database& database);

    void remove_json_connection(const json_sql_connection& conn);
    void remove_json_database(const json_database& database);

    void get_databases(tasker::database_array& array);
}

#endif