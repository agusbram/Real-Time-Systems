import sys # To read stdout of bash script from giroscope.c
import matplotlib.pyplot as plt 
from collections import deque

# Visible samples quantity
N = 100

# Create queue list of FIFO type
x_data = deque([0]*N)
y_data = deque([0]*N)
z_data = deque([0]*N)

# Configuration graph
# Allows to update the graph without blocking it
plt.ion()
fig, ax = plt.subplots()

# Creates curves of graph with their respective labels
line_x, = ax.plot(x_data, label='X')
line_y, = ax.plot(y_data, label='Y')
line_z, = ax.plot(z_data, label='Z')

# Graph it
ax.legend()

# Fixes the vertical range (aceleration in g)
ax.set_ylim(-2, 2)

# 1. Reads new data
# 2. Adds new data into the window
# 3. Updates the graph
while True:
    # Read one line from stdin (from pipe requested in requirements)
    line = sys.stdin.readline()
    
    # If program finishes ==> Breaks this loop
    if not line:
        break

    try:
        # Secure parsing data
        # Converts this: 0.12,0.05,0.98
        # Into this: 
        # fx = 0.12
        # fy = 0.05
        # fz = 0.98
        fx, fy, fz = map(float, line.strip().split(','))
    # Avoids that errors breaks the script
    except:
        continue

    # Updates buffers
    # Maintain sliding window
    # Pushes new data into FIFO list
    # This allows to have exactly N elements and there are the most recent ones
    x_data.append(fx)
    y_data.append(fy)
    z_data.append(fz)

    # Remove first input
    x_data.popleft()
    y_data.popleft()
    z_data.popleft()

    # Updates graph
    # Changes values to the curve
    line_x.set_ydata(x_data)
    line_y.set_ydata(y_data)
    line_z.set_ydata(z_data)

    # Redraw
    # Refreshes graph
    # Allows animation in real time
    plt.draw()

    # Controls updating speed
    # Controls FPS graph (maximum 100 Hz)
    plt.pause(0.01)