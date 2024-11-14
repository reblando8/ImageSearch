#!/bin/bash

# SLURM directives
#SBATCH --account=PMIU0184
#SBATCH --job-name=factor_count
#SBATCH --time=01:00:00
#SBATCH --mem=1GB
#SBATCH --nodes=1
#SBATCH --cpus-per-task=8

# Set number of OpenMP threads based on number of cores specified by SLURM
export OMP_NUM_THREADS=$SLURM_CPUS_PER_TASK

# Load necessary modules (if module system is available)
# module load gcc/9.2.0  # Adjust according to available modules and requirements

# Compile the program
g++ -o my_program main.cpp PNG.cpp -fopenmp -lpng -lz

# Run the program 3 times to measure consistent timings
/usr/bin/time -v ./my_program images/MiamiMarcumCenter.png images/WindowPane_mask.png result.png true 50 64