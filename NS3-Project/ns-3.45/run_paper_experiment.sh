#!/bin/bash
./ns3 build

mkdir -p Results_Fig1

echo "Running Fig 1(a): PLR = 0%"
./ns3 run "scratch/cubic-fit-fig1 --errorRate=0.0 --output_file=Results_Fig1/fig1_a_raw.txt"

echo "Running Fig 1(b): PLR = 1%"
./ns3 run "scratch/cubic-fit-fig1 --errorRate=0.01 --output_file=Results_Fig1/fig1_b_raw.txt"

echo "Plotting Figures..."
python3 plot_fig1.py

echo "Done! The plots are saved as Results_Fig1/fig1_a.png and Results_Fig1/fig1_b.png."
