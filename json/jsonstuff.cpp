#include "jsonstuff.h"
#include "json11.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

//New json library: https://github.com/davidmoreno/json11/

namespace tasker {
    int get_con_pos(Json& config, const tasker::json_sql_connection& connection);

    int add_json_connection(const tasker::json_sql_connection& connection) {
        Json config;
        {
            std::ifstream file("config.json");
            config = Json(file);
        }

        int con_size = get_con_pos(config, connection);

        if(con_size < 0) {
            con_size = Json(config["connections"]).size();
            Json(config["connections"]) << Json::object();
            config["connections"][con_size]["ip"] = connection.ip;
            config["connections"][con_size]["port"] = connection.port;
            config["connections"][con_size]["username"] = connection.username;
            config["connections"][con_size]["password"] = connection.password;
            config["connections"][con_size]["schemas"] = Json::array();
            std::ofstream("config.json") << config;
        }

        return con_size;
    }

    void add_json_database(const tasker::json_database& database) {
        int con_pos = add_json_connection(database.connection);
        Json config;
        {
            std::ifstream file("config.json");
            config = Json(file);
        }

        if(Json(config["connections"][con_pos]["schemas"]).size() != 0)
            for(int i = 0; i < Json(config["connections"][con_pos]["schemas"]).size(); i++) 
                if(config["connections"][con_pos]["schemas"][i] == database.schema) return;

        Json(config["connections"][con_pos]["schemas"]) << database.schema;
        std::ofstream("config.json") << config;
    }

    void remove_json_connection(const tasker::json_sql_connection& connection) {
        Json config;
        {
            std::ifstream file("config.json");
            config = Json(file);
        }

        int con_pos = get_con_pos(config, connection);
        if(con_pos < 0) return;

        Json(config["connections"]).erase(con_pos);
        std::ofstream("config.json") << config;
    }

    void remove_json_database(const tasker::json_database& database) {
        Json config;

        {
            std::ifstream file ("config.json");
            config = Json(file);
        }

        int con_pos = get_con_pos(config, database.connection);

        if(con_pos < 0) return;

        if(Json(config["connections"][con_pos]["schemas"]).size() <= 1) {
            remove_json_connection(database.connection);
            return;
        }

        for(int i = 0; i < Json(config["connections"][con_pos]["schemas"]).size(); i++) {
            if(config["connections"][con_pos]["schemas"][i] == database.schema) {
                Json(config["connections"][con_pos]["schemas"]).erase(i);
                break;
            }
        }

        //config["connections"][con_pos]["schemas"] = schemas;
        std::ofstream("config.json") << config;
    }

    int get_con_pos(Json& config, const tasker::json_sql_connection& connection) {
        if(Json(config["connections"]).size() < 1) return -1;
        int con_pos = -1;
        for(int i = 0; i < Json(config["connections"]).size(); i++) {
            if(config["connections"][i]["ip"] == connection.ip && int(config["connections"][i]["port"]) == connection.port &&
                config["connections"][i]["username"] == connection.username && config["connections"][i]["password"] == connection.password) {
                con_pos = i;
                break;
            }
        }

        return con_pos;
    }

    void get_databases(tasker::database_array& array) {
        Json config;
        {
            std::ifstream file("config.json");
            config = Json(file);
        }
        
        array.databases = std::vector<tasker::json_database>();
        array.connections = std::vector<tasker::json_sql_connection>();

        for(int i = 0; i < Json(config["connections"]).size(); i++) {
            tasker::json_sql_connection connection{config["connections"][i]["ip"], int(config["connections"][i]["port"]),
                config["connections"][i]["username"], config["connections"][i]["password"]};
            for(int j = 0; j < Json(config["connections"][i]["schemas"]).size(); j++) {
                array.databases.push_back(tasker::json_database{connection, config["connections"][i]["schemas"][j]});
            }

            array.connections.push_back(connection);
        }
    }
}