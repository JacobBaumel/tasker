#ifndef JSON_STUFF_H
#define JSON_STUFF_H

#include "RSJParser.h"
#include <iostream>

namespace tasker {

    struct json_sql_connection {
        std::string ip;
        int port;
        std::string username;
        std::string password;
        std::string schema;
    };

    void add_json_connection(const json_sql_connection& conn);

    void remove_json_connection(const json_sql_connection& conn);
}

#endif