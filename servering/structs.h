#ifndef STRUCTS_H
#define STRUCTS_H

#include "imgui.h"
#include <string>
#include <vector>

namespace tasker {
        struct status {
        char name[256] = "";
        ImColor color;

        status(const std::string& _name, const int& r, const int& g, const int& b);
    };

    struct task {
        status* statuss;
        char taskk[256] = "";
        char date[256] = "";
        char people[256] = "";
        int pos;
        int id;
        bool wasSelected = false;

        task(status* _statuss, const std::string& _taskk, const std::string& _date, const std::string& _people, const int& _pos, const int& _id);
    };

    struct supertask {
        std::vector<task*> tasks;
        task* newTask;
        ImColor color;
        char name[256] = "";
        char display_name[256] = "";
        bool collapsed = false;

        supertask(const std::string& _name, const std::string& _display_name, const ImColor& _color);
    };

    struct workspace {
        std::vector<status*> stati;
        std::vector<supertask*> tasks;
        char name[256] = "";
        char new_category[256] = "";
        float new_color[3] = {0, 0, 0};
        bool create_cat = false;
        bool manage_statuses = false;

        workspace(const std::string& _name);

        ~workspace();

        status* get_status(const std::string& name);
    };
}

#endif