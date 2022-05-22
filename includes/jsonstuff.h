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
    };

    struct json_database {
        json_sql_connection connection;
        std::string schema;

        //~json_database() {delete connection;}
    };

    struct database_array {
        std::vector<json_database> databases;
        int size;
    };

    int add_json_connection(const json_sql_connection& conn);
    void add_json_database(const tasker::json_database& database);

    void remove_json_connection(const json_sql_connection& conn);

    void get_databases(tasker::database_array& array);
}

#endif