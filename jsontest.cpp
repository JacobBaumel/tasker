#include <iostream>
#include "json11.h"
#include "db_functions.h"

int main() {
    tasker::workspace config;
    tasker::json_sql_connection conn {"129.159.35.219", 3306, "tasker", "W11LuvR0b07ics!"};
    tasker::set_connection(conn);
    tasker::set_schema("testSpace");
    tasker::get_data(config);
}