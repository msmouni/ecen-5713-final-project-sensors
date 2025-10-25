#ifndef SENSORS_DB_H
#define SENSORS_DB_H

#include <sqlite3.h> // SQLite library

struct sensors_db
{
    sqlite3 *db;
    char *err_msg;
    int data_limit;
};

struct sensors_db *sensors_db_init(char *db_file, int data_limit);

int sensors_db_store_data(struct sensors_db *self, float bmp280_temp, float bmp280_pressure, float htu21d_temp, float htu21d_humidity);

void sensors_db_close(struct sensors_db *self);

#endif /* SENSORS_DB_H */