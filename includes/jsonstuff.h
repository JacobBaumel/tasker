#ifndef JSON_STUFF_H
#define JSON_STUFF_H

#include <iostream>

namespace tasker {

    struct json_sql_connection {
        std::string ip;
        int port;
        std::string username;
        std::string password;
        std::string* schema;
        int schema_number;
    };

    struct json_sql_connection_array {
        json_sql_connection* connections;
        int length;

        ~json_sql_connection_array() {
            for(int i = 0; i < length; i++) {
                delete connections++;
            }
        }

        json_sql_connection* operator[] (int index) {
            return connections + index;
        }
    };

    void add_json_connection(const json_sql_connection& conn);

    void remove_json_connection(const json_sql_connection& conn);

    json_sql_connection_array* get_json_connections();
}

#endif