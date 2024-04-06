#!/bin/bash

# Parámetros comunes para todas las ejecuciones
MPI_COMP_COMMAND="mpicc main.c -o main -lm"
RM_EXE="rm main"
MPIRUN_COMMAND="mpirun --oversubscribe"
MAIN_PROGRAM="main"

$MPI_COMP_COMMAND

$MPIRUN_COMMAND -np 11 $MAIN_PROGRAM 6 4

$RM_EXE
