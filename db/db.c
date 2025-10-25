#include "db.h"
#include <stdio.h>
#include <stdlib.h>

struct sensors_db *sensors_db_init(char *db_file, int data_limit)
{

    struct sensors_db *sens_db = (struct sensors_db *)malloc(sizeof(struct sensors_db));

    if (!sens_db)
    {
        return NULL;
    }

    // Open or create the SQLite database
    int rc = sqlite3_open(db_file, &sens_db->db);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(sens_db->db));
        sqlite3_close(sens_db->db);
        return NULL;
    }

    // Create the table if it doesn't exist
    const char *create_table_sql =
        "CREATE TABLE IF NOT EXISTS SensorData ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
        "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP, "
        "bmp280_temperature REAL, "
        "bmp280_pressure REAL, "
        "htu21d_temperature REAL, "
        "htu21d_humidity REAL);";

    rc = sqlite3_exec(sens_db->db, create_table_sql, 0, 0, &sens_db->err_msg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", sens_db->err_msg);
        sqlite3_free(sens_db->err_msg);
        sqlite3_close(sens_db->db);
        return NULL;
    }

    sens_db->data_limit = data_limit;

    return sens_db;
}

int sensors_db_store_data(struct sensors_db *self, float bmp280_temp, float bmp280_pressure, float htu21d_temp, float htu21d_humidity)
{

    // Insert new sensor data into the database
    char insert_sql[512];
    snprintf(insert_sql, sizeof(insert_sql),
             "INSERT INTO SensorData (bmp280_temperature, bmp280_pressure, htu21d_temperature, htu21d_humidity) "
             "VALUES (%.2f, %.2f, %.2f, %.2f);",
             bmp280_temp, bmp280_pressure, htu21d_temp, htu21d_humidity);

    int rc = sqlite3_exec(self->db, insert_sql, 0, 0, &self->err_msg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", self->err_msg);
        return -1;
    }

    // Delete older entries if more than 10 samples exist
    char delete_sql[512];
    snprintf(delete_sql, sizeof(delete_sql), "DELETE FROM SensorData WHERE id NOT IN (SELECT id FROM SensorData ORDER BY id DESC LIMIT %d);", self->data_limit);
    rc = sqlite3_exec(self->db, delete_sql, 0, 0, &self->err_msg);
    if (rc != SQLITE_OK)
    {
        fprintf(stderr, "SQL error while deleting old data: %s\n", self->err_msg);
    }

    return 0;
}

void sensors_db_close(struct sensors_db *self)
{
    sqlite3_free(self->err_msg);
    sqlite3_close(self->db);
}
