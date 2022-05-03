#include "jsonstuff.h"
#include "RSJParser.h"
#include <fstream>
#include <sstream>
#include <iostream>



namespace tasker {
    void add_json_connection(const tasker::json_sql_connection& connection) {
        std::ifstream file("config.json");
        std::stringstream stream;
        stream << file.rdbuf();
        json::JSON config = json::JSON::Load(stream.str());
        int con_size = 0;
        bool has_entry = false;
        for(int i = 0; i < config["connections"].size(); i++) {
            if(config["connections"][i]["ip"].ToString() == connection.ip && config["connections"][i]["port"].ToInt() == connection.port &&
                config["connections"][i]["username"].ToString() == connection.username && config["connections"][i]["password"].ToString() == connection.password) {
                con_size = i;
                has_entry = true;
                break;
            }
        } 
        if(!has_entry) {
            con_size = config["connections"].size();
            config["connections"][con_size]["ip"] = connection.ip;
        config["connections"][con_size]["port"] = connection.port;
        config["connections"][con_size]["username"] = connection.username;
        config["connections"][con_size]["password"] = connection.password;
        }
        if(connection.schema != "") config["connections"][con_size]["schemas"].append(connection.schema);
        std::ofstream out("config.json");
        out << config;
    }

    void remove_json_connection(const tasker::json_sql_connection& connection) {
        if(connection.ip == "" || connection.username == "" || connection.password == "") return;
        std::ifstream file("config.json");
        std::stringstream stream;
        stream << file.rdbuf();
        json::JSON config = json::JSON::Load(stream.str());
        
        bool has_entry = false;
        int con_pos = 0;
        for(int i = 0; i < config["connections"].size(); i++) {
            if(config["connections"][i]["ip"].ToString() == connection.ip && config["connections"][i]["port"].ToInt() == connection.port &&
                config["connections"][i]["username"].ToString() == connection.username && config["connections"][i]["password"].ToString() == connection.password) {
                con_pos = i;
                has_entry = true;
                break;
            }
        }

        if(!has_entry) return;

        if(connection.schema == "") {
            json::JSON connections = config["connections"];
            std::deque<json::JSON>::iterator it = connections.ArrayRange().begin();
            config["connections"] = json::Array();
            
            for(int i = 0; it < connections.ArrayRange().end(); it++) {
                if(i != con_pos) config["connections"].append(*it);
                i++;
            }
        }

        else {
            json::JSON schemas = config["connections"][con_pos]["schemas"];
            std::deque<json::JSON>::iterator it = schemas.ArrayRange().begin();
            config["connections"][con_pos]["schemas"] = json::Array();

            for(; it < schemas.ArrayRange().end(); it++) {
                if(it->ToString() != connection.schema) config["connections"][con_pos]["schemas"].append(*it);
            }
        }
        
        std::ofstream out("config.json");
        out << config;
    }
}