import matplotlib.pyplot as plt
import csv

# Read data from CSV file where each sweep is separated by 'turn'
data = []
current_sweep = []

with open('input_data.csv', newline='') as csvfile:
    csvreader = csv.reader(csvfile)
    for row in csvreader:
        for value in row:
            if value.strip().lower() == 'turn':  # When encountering 'turn', store the current sweep and reset
                if current_sweep:
                    data.append(current_sweep)  # Add the current sweep to the data list
                    current_sweep = []  # Reset for the next sweep
            elif value.strip():  # Skip empty values
                current_sweep.append(int(value.strip()))  # Add the number to the current sweep

        # After the loop, ensure we add the last sweep if there’s any data left
        if current_sweep:
            data.append(current_sweep)

# Check if we successfully read the data
print(f"Data: {data}")

# Plot Sweep 1 (Base)
plt.plot(range(len(data[0])), data[0], label="Sweep 1", color='blue')

num_scans = len(data[0]) - 1  # Number of scans in data[0]
min_y_value = min(data[0])  # Minimum y value from data[0]

# Sweep 2 (Right Side - Ascending)
calculated_points2 = []
min_sweep2 = min(data[1])

for i in range(len(data[1])):
    x_value = num_scans - data[1][i] + min_sweep2  # Adjust x value
    y_value = min_y_value + i  # Ascending y value
    calculated_points2.append((x_value, y_value))

x_points2, y_points2 = zip(*calculated_points2)
plt.plot(x_points2, y_points2, marker='o', color='red', label='Sweep 2')

# **Sweep 3 (Top Half - Descending)**
calculated_points3 = []
max_sweep3 = max(data[2])
num_scans = len(data[1]) - 1
for i in range(len(data[2])):
    x_value = num_scans - data[1][i] + min(data[1])
    x_value = i  # Move left from the last Sweep 2 x value
    y_value = y_points2[-1] - (data[2][i] - min_y_value)  # Descending y-value
    calculated_points3.append((x_value, y_value))

x_points3, y_points3 = zip(*calculated_points3)
plt.plot(x_points3, y_points3, marker='o', color='green', label='Sweep 3')

# **Sweep 4 (Top Half - Descending)**
calculated_points4 = []
min_sweep4 = min(data[3])  # Get the minimum value for correct mirroring

for i in range(len(data[3])):
    x_value = data[3][i] - min_sweep4  # Mirror Sweep 2
    y_value = i + min_y_value  # Adjust y to ascend correctly
    calculated_points4.append((x_value, y_value))

x_points4, y_points4 = zip(*calculated_points4)

plt.plot(x_points4, y_points4, marker='o', color='yellow', label='Sweep 4')

# Adding labels and title
plt.xlabel('X-axis')
plt.ylabel('Y-axis')
plt.title('Collected Data')

# Adding a legend
plt.legend()

# Display the plot
plt.show()
