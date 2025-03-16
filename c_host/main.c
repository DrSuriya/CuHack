// you initially have to start the system when the object is already in view of the sensor

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/neutrino.h>
#include "2_l298n_driver.c"
#include "rpi_gpio.h"

#define GPIO_PULSE_PIN 18
#define GPIO_INTERRUPT_PIN 25
#define SPEED_OF_SOUND_CM_PER_US 0.0343
#define EDGE_TIMEOUT_MS 100.0
#define RPI_PERIPHERAL_BASE 0xfe000000

volatile uint32_t *__RPI_GPIO_REGS = NULL;

static float calculate_elapsed_time_us(const struct timespec *start, const struct timespec *stop)
{
    return (stop->tv_sec - start->tv_sec) * 1e6 +
           (stop->tv_nsec - start->tv_nsec) / 1e3;
}

static int wait_for_gpio_state(int gpio_pin, bool desired_state, double timeout_ms, struct timespec *timestamp)
{
    struct timespec start, current;
    clock_gettime(CLOCK_REALTIME, &start);

    while (1)
    {
        bool current_state = (rpi_gpio_read(gpio_pin) != 0);
        if (current_state == desired_state)
        {
            clock_gettime(CLOCK_REALTIME, timestamp);
            return 0;
        }
        clock_gettime(CLOCK_REALTIME, &current);
        double elapsed_ms = (current.tv_sec - start.tv_sec) * 1000.0 +
                            (current.tv_nsec - start.tv_nsec) / 1e6;
        if (elapsed_ms > timeout_ms)
        {
            return -1;
        }
    }
}

static int send_pulse(void)
{
    printf("Sending trigger pulse...\n");
    rpi_gpio_set(GPIO_PULSE_PIN);

    struct timespec pulse_duration = {.tv_sec = 0, .tv_nsec = 10 * 1000};
    nanosleep(&pulse_duration, NULL);

    rpi_gpio_clear(GPIO_PULSE_PIN);
    return 0;
}

static bool init_gpios(void)
{
    if (!rpi_gpio_map_regs(RPI_PERIPHERAL_BASE))
    {
        printf("Failed to map GPIO registers\n");
        return false;
    }

    rpi_gpio_set_select(GPIO_PULSE_PIN, RPI_GPIO_FUNC_OUT);
    rpi_gpio_set_select(GPIO_INTERRUPT_PIN, RPI_GPIO_FUNC_IN);

    if (!rpi_gpio_set_pud_bcm2711(GPIO_INTERRUPT_PIN, RPI_GPIO_PUD_OFF))
    {
        printf("Failed to disable pull resistor on echo pin\n");
        return false;
    };

    __RPI_GPIO_REGS[RPI_GPIO_REG_GPEDS0] = 0xFFFFFFFF;

    return true;
}

static int read_distance(float *distance)
{
    if (send_pulse() != 0)
    {
        printf("Failed to send trigger pulse\n");
        return -1;
    }

    struct timespec rising_edge_time, falling_edge_time;

    if (wait_for_gpio_state(GPIO_INTERRUPT_PIN, true, EDGE_TIMEOUT_MS, &rising_edge_time) != 0)
    {
        printf("Timeout waiting for rising edge\n");
        return -1;
    }
    printf("Rising edge detected\n");

    if (wait_for_gpio_state(GPIO_INTERRUPT_PIN, false, EDGE_TIMEOUT_MS, &falling_edge_time) != 0)
    {
        printf("Timeout waiting for falling edge\n");
        return -1;
    }
    printf("Falling edge detected\n");

    float pulse_duration_us = calculate_elapsed_time_us(&rising_edge_time, &falling_edge_time);

    *distance = (pulse_duration_us * SPEED_OF_SOUND_CM_PER_US) / 2.0;

    return 0;
}

void change_direction(const char *direction, int distance_buffer, FILE *fpt)
{
    const char *newDirection = direction;
    if (strcmp(direction, "North") == 0)
    {
        newDirection = "East";
    }
    else if (strcmp(*direction, "East") == 0)
    {
        newDirection = "South";
    }
    else if (strcmp(*direction, "South") == 0)
    {
        newDirection = "West";
    }
    else if (strcmp(*direction, "West") == 0)
    {
        newDirection = "North";
    }
 
    printf("sending signal to motor to rotate left by 90 degrees\n");
    distance_buffer++;
    
    if (distance_buffer == 4)
    {
        distance_buffer = 0;
        fseek(fpt, 0, SEEK_SET);
        fprintf(fpt, "%s\n", newDirection);
    }   
    fprintf(fpt, "Changing Direction to %s.", newDirection);
    
}

int main(void)
{
    signal(SIGINT, sigint_handler);

	printf("=== L298N Motor Driver PWM Running ===\n");

	// Initialize the motor driver.
	if (motor_init() != GPIO_SUCCESS) {
		printf("ERROR: Motor initialization failed.\n");
		return EXIT_FAILURE;
	}

	// Set both motors for forward drive.
	motor_right_forward();
	motor_left_forward();

    struct _clockperiod period;
    period.nsec = 10000;
    period.fract = 0;
    if (ClockPeriod(CLOCK_REALTIME, NULL, &period, 0) == -1)
    {
        perror("ClockPeriod");
        return EXIT_FAILURE;
    }

    if (!init_gpios())
    {
        printf("Failed to initialize GPIOs\n");
        return EXIT_FAILURE;
    }

    FILE *fpt;
    fpt = fopen("sensor_data.csv", "w+");
    if (fpt == NULL)
    {
        printf("Failed to open file\n");
        return EXIT_FAILURE;
    }

    int distance_buffer = 3; // the sensor is placed in front of the object, once the system starts this gets 

    char *direction = "North"; // just default direction doesnt have to be reflective of real life

    /* sleep(2); */
	motor_set_speed((float)40, MOTOR_ENA_PIN);
	motor_set_speed((float)40, MOTOR_ENB_PIN);

    while (1)
    {
        static bool notReady = false;
        float distance;
        if (read_distance(&distance) == 0)
        {
            sleep(1);
            printf("Distance: %.2f cm\n", distance);
            if (notReady || distance > 100.00) {
                fflush(fpt);
            }
            else {
                fprintf(fpt, "%.2f\n", distance);
                fflush(fpt);
            }
            
            if (distance > 100.00 && notReady != true)
            {
                if (distance_buffer == 4) {

                    distance_buffer = 0;
                    fseek(fpt, 0, SEEK_SET);
                    fprintf(fpt, "turn\n");
                }
                else {
                    distance_buffer++; 
                    fprintf(fpt, "turn\n");
                }
                notReady = true;
                motor_turn_left();
                sleep(1.5);
                motor_right_forward();
                motor_left_forward();
                /* change_direction(direction, distance_buffer, fpt); */
                /* sleep(1); */
            }
            else if (distance > 100) {
                notReady = true;
            }
            else {
                notReady = false;
            }
        }
        else
        {
            printf("Error reading distance\n");
        }

        /* usleep(100000); */
    }
    fclose(fpt);
    return EXIT_SUCCESS;
}
