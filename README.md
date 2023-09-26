# Thesis : Efficient rendering of SPH based fluid simulations


## Overview

This project is a fluid simulation implemented in C++ 17 using the Vulkan API. The SPH (Smoothed Particle Hydrodynamics) method is used to simulate the fluid, and a bounding box is created to contain the fluid particles. The particles are represented as spheres for now.

## Methodology

SPH is a widely used method for simulating fluid flows. It is a Lagrangian method that solves the Navier-Stokes equations by approximating the fluid as a set of discrete particles. The fluid is simulated by computing the pressure, density, and velocity of each particle based on the interaction with its neighbors.

## Tools

The project is implemented in C++ programming language and uses the Vulkan API for graphics rendering. The GLFW library is used for window and input management, and the GLM library is used for vector and matrix operations.

## Conclusion

In conclusion, this project aims to provide an interactive and visually appealing simulation of fluid dynamics using Vulkan and the SPH method. It provides a good introduction to fluid simulation and the use of Vulkan for graphics programming.

## Setup

To run this project, you need to have the following installed:
- C++ compiler
- Vulkan SDK
- GLFW submodule
- GLM submodule
- ImGui submodule
- Visual Stuido 2022

## Usage

Once the project is built, run the executable FluidSimulationSPH, it can be found in the build directory. The simulation window will open, and you will see the fluid particles moving inside the bounding box.

## Credits

This project was developed by András Tóth. Special thanks to my consultant András Máté Fridvalszky for his guidance and support.
