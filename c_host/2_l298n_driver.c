#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>
#include <signal.h>
#include "rpi_gpio.c"

#define MOTOR_ENA_PIN   12   ///< DRV8833 Enable/nSLEEP pin (PWM controlled)
#define MOTOR_ENB_PIN   13   ///< DRV8833 Enable/nSLEEP pin (PWM controlled)
#define MOTOR_RP_PIN    24   ///< Right Motor "Forward" input (AIN1)
#define MOTOR_RN_PIN    23    ///< Right Motor "Reverse" input (AIN2)
#define MOTOR_LN_PIN    17    ///< Left Motor "Forward" input (BIN1)
#define MOTOR_LP_PIN    22   ///< Left Motor "Reverse" input (BIN2)

// PWM frequency (in Hz) via rpi_gpio.
#define PWM_FREQ 1000

static int setup_pwm_pin(int gpio_pin)
{
    int rc;

    // Set up the GPIO pin as an output.
    rc = rpi_gpio_setup(gpio_pin, GPIO_OUT);
    if (rc != GPIO_SUCCESS) {
        printf("ERROR: rpi_gpio_setup() failed for pin %d, rc=%d\n", gpio_pin, rc);
        return rc;
    }

    // Initialize PWM on this pin using Mark/Space mode at PWM_FREQ.
    rc = rpi_gpio_setup_pwm(gpio_pin, PWM_FREQ, GPIO_PWM_MODE_MS);
    if (rc != GPIO_SUCCESS) {
        printf("ERROR: rpi_gpio_setup_pwm() failed for pin %d, rc=%d\n", gpio_pin, rc);
        return rc;
    }

    // Set the initial PWM duty cycle to 0% (motor off).
    rc = rpi_gpio_set_pwm_duty_cycle(gpio_pin, 0.0f);
    if (rc != GPIO_SUCCESS) {
        printf("ERROR: rpi_gpio_set_pwm_duty_cycle() failed for pin %d, rc=%d\n", gpio_pin, rc);
        return rc;
    }

    return GPIO_SUCCESS;
}

//Configures MOTOR_ENA_PIN to accept PWM input
//Configures MOTOR_ENB_PIN to accept PWM input
//Configures all direction inputs 
int motor_init(void)
{
    int rc;
    printf("[motor_init] Initializing motor driver...\n");

    // Set up the enable pin for PWM control.
    // IMPORTANT: MOTOR_ENA_PIN must be configured for PWM to allow speed control.
    // IMPORTANT: MOTOR_ENB_PIN must be configured for PWM to allow speed control.
    rc = setup_pwm_pin(MOTOR_ENA_PIN);
    rc = setup_pwm_pin(MOTOR_ENB_PIN);
    if (rc != GPIO_SUCCESS)
        return rc;

    // Set up the motor direction pins as digital outputs.
    if (rpi_gpio_setup(MOTOR_RP_PIN, GPIO_OUT) != GPIO_SUCCESS ||
        rpi_gpio_setup(MOTOR_RN_PIN, GPIO_OUT) != GPIO_SUCCESS ||
        rpi_gpio_setup(MOTOR_LP_PIN, GPIO_OUT) != GPIO_SUCCESS ||
        rpi_gpio_setup(MOTOR_LN_PIN, GPIO_OUT) != GPIO_SUCCESS) {
        printf("ERROR: Failed to setup one or more direction pins.\n");
        return -1;
    }

    // Initialize all direction pins to LOW to ensure the motors are off.
    rpi_gpio_output(MOTOR_RP_PIN, GPIO_LOW);
    rpi_gpio_output(MOTOR_RN_PIN, GPIO_LOW);
    rpi_gpio_output(MOTOR_LP_PIN, GPIO_LOW);
    rpi_gpio_output(MOTOR_LN_PIN, GPIO_LOW);

    printf("[motor_init] Motor driver initialization complete.\n");
    return GPIO_SUCCESS;
}

int motor_set_speed(float speed_percent, int motor_pin)
{
    // Set the PWM duty cycle on the enable pin to control motor speed.
    return rpi_gpio_set_pwm_duty_cycle(motor_pin, speed_percent);
}

int motor_disable()
{
    // Disable the motor by setting the PWM duty cycle to 0%.
    motor_set_speed(0.0f, MOTOR_ENA_PIN);
    return motor_set_speed(0.0f, MOTOR_ENB_PIN);
}

void sigint_handler(int sig)
{
    (void)sig;  // Unused parameter
    printf("\nSIGINT received, disabling motor driver...\n");
    motor_disable();
    exit(EXIT_SUCCESS);
}

//Sets the right motor to move forward.
int motor_right_forward(void)
{
    rpi_gpio_output(MOTOR_RP_PIN, GPIO_HIGH);
    rpi_gpio_output(MOTOR_RN_PIN, GPIO_LOW);
    return GPIO_SUCCESS;
}

//Sets the right motor to move reverse.
int motor_right_reverse(void)
{
    rpi_gpio_output(MOTOR_RP_PIN, GPIO_LOW);
    rpi_gpio_output(MOTOR_RN_PIN, GPIO_HIGH);
    return GPIO_SUCCESS;
}

//Sets the left motor to move forward.
int motor_left_forward(void)
{
    rpi_gpio_output(MOTOR_LP_PIN, GPIO_LOW);
    rpi_gpio_output(MOTOR_LN_PIN, GPIO_HIGH);
    return GPIO_SUCCESS;
}

//Sets the left motor to move reverse.
int motor_left_reverse(void)
{
    rpi_gpio_output(MOTOR_LP_PIN, GPIO_HIGH);
    rpi_gpio_output(MOTOR_LN_PIN, GPIO_LOW);
    return GPIO_SUCCESS;
}

//Stops both motors 
int motor_stop(void)
{
    rpi_gpio_output(MOTOR_RP_PIN, GPIO_LOW);
    rpi_gpio_output(MOTOR_RN_PIN, GPIO_LOW);
    rpi_gpio_output(MOTOR_LP_PIN, GPIO_LOW);
    rpi_gpio_output(MOTOR_LN_PIN, GPIO_LOW);
    motor_set_speed(0.0f, MOTOR_ENA_PIN);
    motor_set_speed(0.0f, MOTOR_ENB_PIN);
    return GPIO_SUCCESS;
}

//Turns car left
int motor_turn_left(void)
{
    motor_left_reverse();
    motor_right_forward();
    return GPIO_SUCCESS;
}

//Turns car right
int motor_turn_right(void)
{
    motor_left_forward();
    motor_right_reverse();
    return GPIO_SUCCESS;
}



/* int main() { */

/*     int speed; */

/*     // SIGINT handler to catch Ctrl+C and disable the motors safely. */
/*     signal(SIGINT, sigint_handler); */

/*     printf("=== DRV8833 Motor Driver PWM Demo (Enable Pin Speed Control) ===\n"); */

/*     // Initialize the motor driver. */
/*     if (motor_init() != GPIO_SUCCESS) { */
/*         printf("ERROR: Motor initialization failed.\n"); */
/*         return EXIT_FAILURE; */
/*     } */

/*     // Set both motors for forward drive. */
/*     motor_right_forward(); */
/*     motor_left_reverse(); */
/*     motor_set_speed((float)100, MOTOR_ENA_PIN); */
/*     motor_set_speed((float)100, MOTOR_ENB_PIN); */
/*     sleep(3); */

/*     /1* printf("\n--- Ramping speed from 0%% to 100%% ---\n"); *1/ */
/*     /1* for (speed = 0; speed <= 50; speed += 10) { *1/ */
/*     /1*     printf("Setting speed to %d%%\n", speed); *1/ */
/*     /1*     /2* motor_set_speed((float)speed, MOTOR_ENB_PIN); *2/ *1/ */
/*     /1*     motor_set_speed((float)speed, MOTOR_ENA_PIN); *1/ */
/*     /1*     sleep(2); *1/ */
/*     /1* } *1/ */

/*     /1* printf("\n--- Ramping speed from 100%% down to 0%% ---\n"); *1/ */
/*     /1* for (speed = 50; speed >= 0; speed -= 10) { *1/ */
/*     /1*     printf("Setting speed to %d%%\n", speed); *1/ */
/*     /1*     /2* motor_set_speed((float)speed, MOTOR_ENB_PIN); *2/ *1/ */
/*     /1*     motor_set_speed((float)speed, MOTOR_ENA_PIN); *1/ */
/*     /1*     sleep(2); *1/ */
/*     /1* } *1/ */

/*     printf("\n--- Disabling motor driver ---\n"); */
/*     motor_disable(); */

/*     printf("=== Test Complete ===\n"); */
/*     return EXIT_SUCCESS; */

/* } */
