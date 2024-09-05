import numpy as np
import matplotlib.pyplot as plt
from matplotlib.patches import RegularPolygon, Rectangle
import matplotlib.patches as mpatches
from time import sleep
arrows = []
fig, ax = plt.subplots()
legend_patches = []

def calculate_wheel_velocities(theta_robot, theta_desired, v, R):
    # Calculate velocity components in the robot frame
    theta_robot = np.radians(theta_robot)
    theta_desired = np.radians(theta_desired)
    v_x = v * np.cos(theta_desired - theta_robot)
    v_y = v * np.sin(theta_desired - theta_robot)
    
    # Define wheel orientations relative to the robot's forward direction
    theta_1 = -theta_robot - np.radians(60)  # 60 degrees counterclockwise from robot's forward
    theta_2 = -theta_robot + np.radians(60)  # Forward direction of the robot
    theta_3 = -theta_robot  # 60 degrees clockwise from robot's forward
    
    # Calculate the component of the velocity for each wheel
    # The contribution of each wheel's distance from the center
    v_1 = (v_x - R * np.sin(theta_1)) * np.cos(theta_1) + (v_y + R * np.cos(theta_1)) * np.sin(theta_1)
    v_2 = (v_x - R * np.sin(theta_2)) * np.cos(theta_2) + (v_y + R * np.cos(theta_2)) * np.sin(theta_2)
    v_3 = (v_x - R * np.sin(theta_3)) * np.cos(theta_3) + (v_y + R * np.cos(theta_3)) * np.sin(theta_3)
    
    return v_1, v_2, v_3

wheel_angles = [-60, 60, 180]  # Degrees
r = 1  # Distance from the center to the wheels
wheel_width = 1
wheel_height = 0.3
offset = 0.3  # Offset for the velocity arrow

# Function to plot the robot and its wheel velocity vectors
def plot_robot(orientation, theta, speed, omega):
    # Calculate wheel velocities
    v_wheel_1, v_wheel_2, v_wheel_3 = calculate_wheel_velocities(orientation, theta, speed, R=r)

    # Wheel positions relative to the robot center (aligned with hexagon edges)
    wheel_positions = [
        (r * np.cos(np.radians(30)), r * np.sin(np.radians(30))),    # Wheel 1
        (r * np.cos(np.radians(150)), r * np.sin(np.radians(150))),  # Wheel 2
        (r * np.cos(np.radians(270)), r * np.sin(np.radians(270))),  # Wheel 3
    ]
    ax.set_aspect('equal')

    # Draw robot platform (hexagon)
    hexagon = RegularPolygon((0, 0), numVertices=6, radius=r, orientation=np.radians(30),
                             edgecolor='black', facecolor='lightgray')
    ax.add_patch(hexagon)
    vx = np.cos(np.radians(theta))*speed
    vy = np.sin(np.radians(theta))*speed
    arrows.append(ax.arrow(0, 0, vx*speed, vy*speed, head_width=0.1, head_length=0.2, fc='red', ec='red'))
    # Draw wheels and velocity vectors
    for _, (pos, angle, v_wheel) in enumerate(zip(wheel_positions, wheel_angles, [v_wheel_1, v_wheel_2, v_wheel_3])):
        # Adjust the position so the center of the wheel is on the hexagon edge
        wheel_center_x = pos[0]
        wheel_center_y = pos[1]
        rect_x = wheel_center_x - (wheel_width / 2) * np.cos(np.radians(angle))
        rect_y = wheel_center_y - (wheel_width / 2) * np.sin(np.radians(angle))

        # Draw wheel as a rectangle
        rect = Rectangle((rect_x, rect_y), 
                         width=wheel_width, height=wheel_height, angle=angle, 
                         edgecolor='black', facecolor='darkgray')
        ax.add_patch(rect)

        # Draw velocity vector as an arrow along the width of the wheel
        arrow_start_x = wheel_center_x + (wheel_height / 2 + offset) * np.cos(np.radians(angle + 90))
        arrow_start_y = wheel_center_y + (wheel_height / 2 + offset) * np.sin(np.radians(angle + 90))
        arrow_dx = v_wheel * np.cos(np.radians(angle-90))
        arrow_dy = v_wheel * np.sin(np.radians(angle-90))
        arrows.append(ax.arrow(arrow_start_x, arrow_start_y, arrow_dx, arrow_dy, head_width=0.1, head_length=0.2, fc='red', ec='red'))

    # Create a list of legend patches
    legend_patches.extend([
        mpatches.Patch(color='none'),
        mpatches.Patch(color='none'),
        mpatches.Patch(color='none'),
    ])

    ax.set_xlim(-2, 2)
    ax.set_ylim(-2, 2)
    plt.grid(True)

plot_robot(0, 0, 1, omega=0)
ori = 0
move_deg = 90
move_speed = 1
w = 0
while True:
    if(plt.fignum_exists(fig.number) == False):
       break
    vxx = np.cos(np.radians(move_deg))
    vyy = np.sin(np.radians(move_deg))
    arrows[0].set_data(dx=vxx, dy=vyy)

    v_wheel_1, v_wheel_2, v_wheel_3 = calculate_wheel_velocities(ori, move_deg, move_speed, R=r)
    for i, (angle, v) in enumerate(zip(wheel_angles, [v_wheel_1, v_wheel_2, v_wheel_3])):
        arrow_dx = v * np.cos(np.radians(wheel_angles[i]))
        arrow_dy = v * np.sin(np.radians(wheel_angles[i]))
        arrows[i+1].set_data(dx=arrow_dx, dy=arrow_dy)
    # Creating legend text
    legend_text = [
        f"Wheel 1 Velocity: v1 = {v_wheel_1:.2f}",
        f"Wheel 2 Velocity: v2 = {v_wheel_2:.2f}",
        f"Wheel 3 Velocity: v3 = {v_wheel_3:.2f}",
    ]

    # Create a list of legend patches
    legend_patches[0].set_label(legend_text[0])
    legend_patches[1].set_label(legend_text[1])
    legend_patches[2].set_label(legend_text[2])
    ax.legend(handles=legend_patches, loc='upper right', frameon=False)

    fig.canvas.draw()
    fig.canvas.flush_events()
    plt.pause(0.01) #is necessary for the plot to update for some reason
    move_deg += 5
    sleep(0.1)
