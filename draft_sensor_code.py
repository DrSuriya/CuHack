#!/usr/bin/env python3

import RPi.GPIO as GPIO
import time
import matplotlib.pyplot as plt

# Ultrasonic Sensor Configuration
TRIGGER_PIN = 13
ECHO_PIN = 25
SPEED_OF_SOUND = 34300  # Speed of sound in cm/s

# Motor Configuration (Stepper Motor)
STEP_PIN = 18
DIR_PIN = 17
STEPS_PER_REVOLUTION = 200  # For a 1.8-degree stepper motor
DELAY = 0.001  # Delay between steps (controls motor speed)

# Initialize GPIO
GPIO.setmode(GPIO.BCM)
GPIO.setup(TRIGGER_PIN, GPIO.OUT)
GPIO.setup(ECHO_PIN, GPIO.IN, pull_up_down=GPIO.PUD_OFF)
GPIO.setup(STEP_PIN, GPIO.OUT)
GPIO.setup(DIR_PIN, GPIO.OUT)

# Ensure the trigger pin is low initially
GPIO.output(TRIGGER_PIN, False)
time.sleep(2)  # Allow sensor to settle

class UltrasonicSensor:
    def __init__(self, trigger_pin, echo_pin, timeout=0.05):
        self.trigger_pin = trigger_pin
        self.echo_pin = echo_pin
        self.timeout = timeout

    def send_trigger_pulse(self):
        GPIO.output(self.trigger_pin, True)
        time.sleep(10e-6)  # 10 microseconds
        GPIO.output(self.trigger_pin, False)

    def get_distance(self):
        self.send_trigger_pulse()

        # Wait for the echo pin to go HIGH (rising edge)
        start_time = time.time()
        while GPIO.input(self.echo_pin) == 0:
            if time.time() - start_time > self.timeout:
                print("Timeout waiting for rising edge")
                return None
        pulse_start = time.time()

        # Wait for the echo pin to go LOW (falling edge)
        while GPIO.input(self.echo_pin) == 1:
            if time.time() - pulse_start > self.timeout:
                print("Timeout waiting for falling edge")
                return None
        pulse_end = time.time()

        pulse_duration = pulse_end - pulse_start
        distance = (pulse_duration * SPEED_OF_SOUND) / 2.0
        return distance

class MotorControl:
    def __init__(self, step_pin, dir_pin):
        self.step_pin = step_pin
        self.dir_pin = dir_pin

    def move_angle(self, angle):
        steps = int((angle / 360) * STEPS_PER_REVOLUTION)
        GPIO.output(self.dir_pin, GPIO.HIGH if angle > 0 else GPIO.LOW)
        for _ in range(abs(steps)):
            GPIO.output(self.step_pin, GPIO.HIGH)
            time.sleep(DELAY)
            GPIO.output(self.step_pin, GPIO.LOW)
            time.sleep(DELAY)

def map_object(sensor, motor, step_angle=10):
    data = []
    for angle in range(0, 360, step_angle):
        motor.move_angle(step_angle)  # Move motor to next angle
        time.sleep(1)  # Wait for motor to stabilize
        distance = sensor.get_distance()
        if distance is not None:
            print(f"Angle: {angle}, Distance: {distance:.2f} cm")
            data.append((angle, distance))
        else:
            print(f"Error measuring distance at angle {angle}.")
    return data

def plot_data(data):
    angles = [point[0] for point in data]
    distances = [point[1] for point in data]

    # Convert polar coordinates (angle, distance) to Cartesian coordinates (x, y)
    x = [dist * (angle / 180 * 3.14159) for dist, angle in zip(distances, angles)]
    y = distances

    plt.figure()
    plt.scatter(x, y)
    plt.title("2D Object Mapping")
    plt.xlabel("X Position (cm)")
    plt.ylabel("Y Position (cm)")
    plt.grid(True)
    plt.show()

def main():
    sensor = UltrasonicSensor(TRIGGER_PIN, ECHO_PIN)
    motor = MotorControl(STEP_PIN, DIR_PIN)

    try:
        print("Starting object mapping...")
        data = map_object(sensor, motor, step_angle=10)  # Map object in 10-degree steps
        print("Mapping complete. Plotting data...")
        plot_data(data)
    except KeyboardInterrupt:
        print("Mapping stopped by user.")
    finally:
        GPIO.cleanup()

if __name__ == '__main__':
    main()
