#include "jsonstuff.h"
#include "RSJParser.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <vector>

namespace tasker {
    int get_con_pos(json::JSON& config, const tasker::json_sql_connection& connection);

    int add_json_connection(const tasker::json_sql_connection& connection) {
        json::JSON config;
        
        {
            std::ifstream file("config.json");
            std::stringstream stream;
            stream << file.rdbuf();
            config = json::JSON::Load(stream.str());
        }

        int con_size = get_con_pos(config, connection);

        if(con_size < 0) {
            con_size = config["connections"].size();
            config["connections"][con_size]["ip"] = connection.ip;
            config["connections"][con_size]["port"] = connection.port;
            config["connections"][con_size]["username"] = connection.username;
            config["connections"][con_size]["password"] = connection.password;
            config["connections"][con_size]["schemas"] = json::Array();
        std::ofstream("config.json") << config;
        }

        return con_size;
    }

    void add_json_database(const tasker::json_database& database) {
        int con_pos = add_json_connection(database.connection);

        json::JSON config;
        
        {
            std::ifstream file("config.json");
            std::stringstream stream;
            stream << file.rdbuf();
            config = json::JSON::Load(stream.str());
        }

        if(config["connections"][con_pos]["schemas"].size() != 0)
            for(int i = 0; i < config["connections"][con_pos]["schemas"].size(); i++) 
                if(config["connections"][con_pos]["schemas"][i].ToString() == database.schema) return;

        config["connections"][con_pos]["schemas"].append(database.schema);

        std::ofstream("config.json") << config;
    }

    void remove_json_connection(const tasker::json_sql_connection& connection) {
        if(connection.ip == "" || connection.username == "" || connection.password == "") return;
        json::JSON config;
        {
            std::ifstream file("config.json");
            std::stringstream stream;
            stream << file.rdbuf();
            config = json::JSON::Load(stream.str());
        }

        int con_pos = get_con_pos(config, connection);

        if(con_pos < 0) return;

        json::JSON connections = config["connections"];
        std::deque<json::JSON>::iterator it = connections.ArrayRange().begin();
        config["connections"] = json::Array();
            
        for(int i = 0; it < connections.ArrayRange().end(); it++) {
           if(i != con_pos) config["connections"].append(*it);
            i++;
        }
        
        std::ofstream("config.json") << config;
    }

    void remove_json_database(const tasker::json_database& database) {
        json::JSON config;
        {
            std::ifstream file("config.json");
            std::stringstream stream;
            stream << file.rdbuf();
            config = json::JSON::Load(stream.str());
        }

        int con_pos = get_con_pos(config, database.connection);

        if(config["connections"][con_pos]["schemas"].size() <= 1) {
            remove_json_connection(database.connection);
            return;
        }

        json::JSON schemas = config["connections"][con_pos]["schemas"];
        std::deque<json::JSON>::iterator it = schemas.ArrayRange().begin();
        config["connections"][con_pos]["schemas"] = json::Array();

        for(; it < schemas.ArrayRange().end(); it++) {
            if(it->ToString() != database.schema) config["connections"][con_pos]["schemas"].append(*it);
        }

        config["connections"][con_pos]["schemas"] = schemas;
        std::ofstream("config.json") << config;
    }

    int get_con_pos(json::JSON& config, const tasker::json_sql_connection& connection) {
        int con_pos = -1;
        for(int i = 0; i < config["connections"].size(); i++) {
            if(config["connections"][i]["ip"].ToString() == connection.ip && config["connections"][i]["port"].ToInt() == connection.port &&
                config["connections"][i]["username"].ToString() == connection.username && config["connections"][i]["password"].ToString() == connection.password) {
                con_pos = i;
                break;
            }
        }

        return con_pos;
    }

    void get_databases(tasker::database_array* array) {
        json::JSON config;
        {
            std::ifstream file("config.json");
            std::stringstream stream;
            stream << file.rdbuf();
            config = json::JSON::Load(stream.str());
        }
        
        array->databases = std::vector<tasker::json_database>();
        array->connections = std::vector<tasker::json_sql_connection>();

        for(int i = 0; i < config["connections"].size(); i++) {
            tasker::json_sql_connection connection{config["connections"][i]["ip"].ToString(), int(config["connections"][i]["port"].ToInt()),
                config["connections"][i]["username"].ToString(), config["connections"][i]["password"].ToString()};
            for(int j = 0; j < config["connections"][i]["schemas"].size(); j++) {
                std::string schema = config["connections"][i]["schemas"][j].ToString();
                array->databases.push_back(tasker::json_database{connection, schema});
            }

            array->connections.push_back(connection);
        }
    }
}