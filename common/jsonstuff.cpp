#include "jsonstuff.h"
#include "json11.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

//json library: https://github.com/davidmoreno/json11/

namespace tasker {
    int get_con_pos(Json& config, const tasker::json_sql_connection& connection);

    // Add just the ip + login to the json file
    int add_json_connection(const tasker::json_sql_connection& connection) {
        // Grab already existing json data
        Json config;
        {
            std::ifstream file("config.json");
            config = Json(file);
        }

        // Get the position of the connection in the config file if it exists
        int con_size = get_con_pos(config, connection);

        // If the connection does not yet exist in json, add it
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

        // Returns con_size so it can be used later
        return con_size;
    }

    // Add the actual database name, and connection (if not already existing) to json
    void add_json_database(const tasker::json_database& database) {
        // Determin con_pos by adding the json connection if it does not exist
        int con_pos = add_json_connection(database.connection);

        // Grab existing json data
        Json config;
        {
            std::ifstream file("config.json");
            config = Json(file);
        }

        // If the database has already been added, return
        if(Json(config["connections"][con_pos]["schemas"]).size() != 0)
            for(int i = 0; i < Json(config["connections"][con_pos]["schemas"]).size(); i++) 
                if(config["connections"][con_pos]["schemas"][i] == database.schema) return;

        // If not, add it to the array at root.connections.con_pos.schemas
        Json(config["connections"][con_pos]["schemas"]) << database.schema;
        std::ofstream("config.json") << config;
    }

    void remove_json_connection(const tasker::json_sql_connection& connection) {
        // Grab existing json data
        Json config;
        {
            std::ifstream file("config.json");
            config = Json(file);
        }

        // Get the position of the required connection, if it exists
        int con_pos = get_con_pos(config, connection);
        if(con_pos < 0) return;

        // If it does exist, erase it from connection
        Json(config["connections"]).erase(con_pos);
        std::ofstream("config.json") << config;
    }

    void remove_json_database(const tasker::json_database& database) {
        // Grab existing json
        Json config;
        {
            std::ifstream file ("config.json");
            config = Json(file);
        }

        // Get the position of the connection
        int con_pos = get_con_pos(config, database.connection);

        if(con_pos < 0) return;

        // If there is only 1 schema in the json (which would be the selected one), remove the connection all together (this could be removed)
        if(Json(config["connections"][con_pos]["schemas"]).size() <= 1) {
            remove_json_connection(database.connection);
            return;
        }

        // Find the index of the schema string, and erase it from the array
        for(int i = 0; i < Json(config["connections"][con_pos]["schemas"]).size(); i++) {
            if(config["connections"][con_pos]["schemas"][i] == database.schema) {
                Json(config["connections"][con_pos]["schemas"]).erase(i);
                break;
            }
        }

        std::ofstream("config.json") << config;
    }

    // Get the position of the requested connection in the json file
    int get_con_pos(Json& config, const tasker::json_sql_connection& connection) {
        if(Json(config["connections"]).size() < 1) return -1;
        int con_pos = -1;

        // Iterate through all connections, and compare info to see if it is the same
        for(int i = 0; i < Json(config["connections"]).size(); i++) {
            if(config["connections"][i]["ip"] == connection.ip && int(config["connections"][i]["port"]) == connection.port &&
                config["connections"][i]["username"] == connection.username && config["connections"][i]["password"] == connection.password) {
                con_pos = i;
                break;
            }
        }

        return con_pos;
    }

    // Load databases from json into object in memory
    void get_databases(tasker::database_array& array) {
        // Grab existing json
        Json config;
        {
            std::ifstream file("config.json");
            config = Json(file);
        }
        
        // Initialize the parts of the array
        array.databases = std::vector<tasker::json_database>();
        array.connections = std::vector<tasker::json_sql_connection>();

        // Iterate through all connections and databases, and load them into the array
        for(int i = 0; i < Json(config["connections"]).size(); i++) {
            tasker::json_sql_connection connection{config["connections"][i]["ip"], int(config["connections"][i]["port"]),
                config["connections"][i]["username"], config["connections"][i]["password"]};
            for(int j = 0; j < Json(config["connections"][i]["schemas"]).size(); j++) {
                array.databases.push_back(tasker::json_database{connection, config["connections"][i]["schemas"][j]});
            }

            array.connections.push_back(connection);
        }
    }

    bool json_sql_connection::operator==(const json_sql_connection& other) {
        return ip == other.ip && port == other.port && username == other.username && password == other.password;
    }
}