#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>

#include "htu21d.h"
#include "bmp280.h"
#include "db.h"

#define I2C_BUS "/dev/i2c-1"
#define DB_FILE "/var/lib/rpi_sensors_data/data.db"
#define DB_DATA_SIZE 100

volatile sig_atomic_t keep_running = 1; // Flag for shutdown

void handle_signal(int signal)
{
    printf("\nCaught signal %d. Shutting down...\n", signal);
    keep_running = 0; // Change flag to exit the loop
}

/*
Double Fork Steps

Let’s first look at the “Double Fork” steps before diving into the code. Note, to understand this you need to be comfortable with the concepts of the session, parent process group, process IDs and hierarchy of processes. I’m going to take the steps from a man 7 daemon

    Call fork() so the process can run in the background
    Call setsid() so once we exit from our shell the shell’s session isn’t killed, which would remove our daemon.
    Call fork() again so the process isn’t the process group leader and cannot take a controlling terminal.

Here is a little more details about these 3 steps above for a “Double Fork”.

    Before a fork call the process will be the process group leader in the shell session. Thus, the parent process will be the shell’s process ID and session ID.
    After our first call to fork the parent process will be killed, thus, the child orphaned and the child will be adopted by the init process and the pgid will be 1. The process group and session will remain the same. The child is no longer the process group leader.
    Call setsid which will put us in a new session and make our process the process group leader, session leader and give us no terminal.
*/
void daemonize(void)
{
    pid_t pid;

    // First fork to create a background process
    pid = fork();
    if (pid < 0)
        exit(EXIT_FAILURE); // Fork failed
    if (pid > 0)
        exit(EXIT_SUCCESS); // Parent exits

    // Create a new session and set the child as session leader
    if (setsid() < 0)
        exit(EXIT_FAILURE);

    // Second fork to prevent reacquiring a terminal
    pid = fork();
    if (pid < 0)
        exit(EXIT_FAILURE);
    if (pid > 0)
        exit(EXIT_SUCCESS); // First child exits

    // Set file permissions mask to 0
    umask(0);

    // Change working directory to root to avoid blocking mounts
    chdir("/");

    // Close all open file descriptors
    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
}

// Function to handle sensor reading and storage
void sensors_update(struct bmp280 *bmp280_sens,
                    struct htu21d *htu21d_sens, struct sensors_db *sens_db, struct htu21d_measurement *temperature, struct htu21d_measurement *humidity,
                    float *bmp280_temp, float *bmp280_pressure,
                    int verbose)
{

    if (bmp280_get_measurement(bmp280_sens, bmp280_temp, bmp280_pressure) == 0)
    {
        if (verbose)
        {
            printf("BMP280 temperature: %.2f °C\n", *bmp280_temp);
            printf("BMP280 pressure: %.2f hPa\n", *bmp280_pressure);
        }

        *temperature = htu21d_read_temperature_no_hold(htu21d_sens);
        *humidity = htu21d_read_humidity_no_hold(htu21d_sens);

        if (temperature->is_valid && humidity->is_valid)
        {
            if (verbose)
            {
                printf("HTU21D temperature: %.2f °C\n", temperature->value);
                printf("HTU21D humidity: %.2f %%RH\n", humidity->value);
            }

            if (sensors_db_store_data(sens_db, *bmp280_temp, *bmp280_pressure,
                                      temperature->value, humidity->value) == 0)
            {
                if (verbose)
                    printf("Sensors data stored successfully\n");
            }
        }
        else if (verbose)
        {
            printf("Invalid HTU21D data: temp valid = %d, humidity valid = %d\n",
                   temperature->is_valid, humidity->is_valid);
        }
    }
}

int main(int argc, char *argv[])
{
    int daemon_mode = 0;

    // Parse command line arguments
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-d") == 0)
            daemon_mode = 1;
        else
        {
            fprintf(stderr, "Usage: %s [-d] [-v]\n", argv[0]);
            return EXIT_FAILURE;
        }
    }

    if (daemon_mode)
        daemonize();

    // Set up signal handlers for SIGINT and SIGTERM
    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    struct I2cBus *i2c_bus = i2c_init(I2C_BUS);

    if (!i2c_bus)
    {
        perror("Failed to initialize I2C bus");
        return -1;
    }

    // Initialize sensors
    struct bmp280 *bmp280_sens = bmp280_init(i2c_bus);
    float bmp280_temp, bmp280_pressure;

    struct htu21d *htu21d_sens = htu21d_init(i2c_bus);
    struct htu21d_measurement temperature, humidity;

    // Initialize SQLite database
    struct sensors_db *sens_db = sensors_db_init(DB_FILE, DB_DATA_SIZE);

    // Main measurement loop
    while (keep_running)
    {
        sensors_update(bmp280_sens, htu21d_sens, sens_db, &temperature, &humidity, &bmp280_temp, &bmp280_pressure, verbose);

        sleep(5); // Wait 5 seconds between measurements
    }
    i2c_close(i2c_bus);
    bmp280_close(bmp280_sens);
    htu21d_close(htu21d_sens);
    sensors_db_close(sens_db);

    return 0;
}
