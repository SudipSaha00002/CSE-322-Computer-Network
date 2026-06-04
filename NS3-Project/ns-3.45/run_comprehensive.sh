#!/bin/bash


mkdir -p Results/CUBIC
mkdir -p Results/B_CUBIC_FIT
mkdir -p Results/M_CUBIC_FIT
mkdir -p Results/Comparison
mkdir -p Results/Traces

# Base Command
CMD="./ns3 run scratch/cubic-fit-wireless.cc"

# Helper function to extract metrics
# Usage: extract_metrics <logfile>
extract_metrics() {
    local logfile=$1
    local thr=$(grep "Total Throughput:" $logfile | cut -d' ' -f3 | tr -d '\r')
    local del=$(grep "Avg End-to-End Delay:" $logfile | cut -d' ' -f4 | tr -d '\r') # ns
    local pdr=$(grep "Packet Delivery Ratio:" $logfile | cut -d' ' -f4 | tr -d '\r')
    local drop=$(grep "Packet Drop Ratio:" $logfile | cut -d' ' -f4 | tr -d '\r')
    local fair=$(grep "Jain's Fairness Index:" $logfile | cut -d' ' -f4 | tr -d '\r')
    
    # Handle empty delay to avoid syntax errors
    if [ -z "$del" ] || [ "$del" = "0" ]; then
        del="0"
    fi
    # Convert Delay to ms (ns / 1000000)
    # Using awk for floating point math
    local del_ms=$(awk -v d="$del" 'BEGIN {print d/1000000}')
    
    echo "$thr $del_ms $pdr $drop $fair"
}

# 1. Delay Experiment (Vary P2P Delay: 10ms - 200ms)
echo "Running Delay Experiment..."
rm -f Results/CUBIC/delay_exp.txt Results/B_CUBIC_FIT/delay_exp.txt Results/M_CUBIC_FIT/delay_exp.txt
for d in 10 50 100 150 200; do
    delay="${d}ms"
    echo "  Delay: $delay"
    
    # Standard CUBIC
    $CMD --command-template="%s --p2pDelay=$delay --transport_prot=ns3::TcpCubic --nWifi=6 --nFlows=2 --errorRate=0.001 --output_file=temp_cubic.txt" > temp_cubic.log 2>&1
    metrics=$(extract_metrics temp_cubic.log)
    echo "$d $metrics" >> Results/CUBIC/delay_exp.txt
    
    # Base CUBIC-FIT
    $CMD --command-template="%s --p2pDelay=$delay --transport_prot=ns3::TcpCubicFit --nWifi=6 --nFlows=2 --errorRate=0.001 --output_file=temp_fit_base.txt --ns3::TcpCubicFit::UseModifications=false" > temp_fit_base.log 2>&1
    metrics=$(extract_metrics temp_fit_base.log)
    echo "$d $metrics" >> Results/B_CUBIC_FIT/delay_exp.txt

    # Modified CUBIC-FIT
    $CMD --command-template="%s --p2pDelay=$delay --transport_prot=ns3::TcpCubicFit --nWifi=6 --nFlows=2 --errorRate=0.001 --output_file=temp_fit_mod.txt --ns3::TcpCubicFit::UseModifications=true" > temp_fit_mod.log 2>&1
    metrics=$(extract_metrics temp_fit_mod.log)
    echo "$d $metrics" >> Results/M_CUBIC_FIT/delay_exp.txt
done

# 2. PLR Experiment (Vary Error Rate: 0.000 - 0.020)
echo "Running PLR Experiment..."
rm -f Results/CUBIC/plr_exp.txt Results/B_CUBIC_FIT/plr_exp.txt Results/M_CUBIC_FIT/plr_exp.txt
for p in 0.000 0.005 0.010 0.015 0.020; do
    echo "  PLR: $p"
    
    # Standard CUBIC
    $CMD --command-template="%s --errorRate=$p --p2pDelay=20ms --transport_prot=ns3::TcpCubic --nWifi=6 --nFlows=2 --output_file=temp_cubic.txt" > temp_cubic.log 2>&1
    metrics=$(extract_metrics temp_cubic.log)
    echo "$p $metrics" >> Results/CUBIC/plr_exp.txt
    
    # Base CUBIC-FIT
    $CMD --command-template="%s --errorRate=$p --p2pDelay=20ms --transport_prot=ns3::TcpCubicFit --nWifi=6 --nFlows=2 --output_file=temp_fit_base.txt --ns3::TcpCubicFit::UseModifications=false" > temp_fit_base.log 2>&1
    metrics=$(extract_metrics temp_fit_base.log)
    echo "$p $metrics" >> Results/B_CUBIC_FIT/plr_exp.txt

    # Modified CUBIC-FIT
    $CMD --command-template="%s --errorRate=$p --p2pDelay=20ms --transport_prot=ns3::TcpCubicFit --nWifi=6 --nFlows=2 --output_file=temp_fit_mod.txt --ns3::TcpCubicFit::UseModifications=true" > temp_fit_mod.log 2>&1
    metrics=$(extract_metrics temp_fit_mod.log)
    echo "$p $metrics" >> Results/M_CUBIC_FIT/plr_exp.txt
done

# 3. Flows Experiment (Vary Flows: 2 - 10)
echo "Running Flows Experiment..."
rm -f Results/CUBIC/flows_exp.txt Results/B_CUBIC_FIT/flows_exp.txt Results/M_CUBIC_FIT/flows_exp.txt
for f in 2 4 6 8 10; do
    echo "  Flows: $f"
    
    # Standard CUBIC
    $CMD --command-template="%s --nFlows=$f --p2pDelay=20ms --errorRate=0.001 --transport_prot=ns3::TcpCubic --nWifi=6 --output_file=temp_cubic.txt" > temp_cubic.log 2>&1
    metrics=$(extract_metrics temp_cubic.log)
    echo "$f $metrics" >> Results/CUBIC/flows_exp.txt
    
    # Base CUBIC-FIT
    $CMD --command-template="%s --nFlows=$f --p2pDelay=20ms --errorRate=0.001 --transport_prot=ns3::TcpCubicFit --nWifi=6 --output_file=temp_fit_base.txt --ns3::TcpCubicFit::UseModifications=false" > temp_fit_base.log 2>&1
    metrics=$(extract_metrics temp_fit_base.log)
    echo "$f $metrics" >> Results/B_CUBIC_FIT/flows_exp.txt

    # Modified CUBIC-FIT
    $CMD --command-template="%s --nFlows=$f --p2pDelay=20ms --errorRate=0.001 --transport_prot=ns3::TcpCubicFit --nWifi=6 --output_file=temp_fit_mod.txt --ns3::TcpCubicFit::UseModifications=true" > temp_fit_mod.log 2>&1
    metrics=$(extract_metrics temp_fit_mod.log)
    echo "$f $metrics" >> Results/M_CUBIC_FIT/flows_exp.txt
done

# 4. Coverage/Range Experiment (Vary Range: 100 - 500)
# Note: nWifi=6 is fairly dense for 100m, sparse for 500m
echo "Running Range Experiment..."
rm -f Results/CUBIC/range_exp.txt Results/B_CUBIC_FIT/range_exp.txt Results/M_CUBIC_FIT/range_exp.txt
for r in 100 200 300 400 500; do
    echo "  Range: $r"
    
    # Standard CUBIC
    $CMD --command-template="%s --range=$r --p2pDelay=20ms --errorRate=0.001 --transport_prot=ns3::TcpCubic --nWifi=6 --nFlows=2 --output_file=temp_cubic.txt" > temp_cubic.log 2>&1
    metrics=$(extract_metrics temp_cubic.log)
    echo "$r $metrics" >> Results/CUBIC/range_exp.txt
    
    # Base CUBIC-FIT
    $CMD --command-template="%s --range=$r --p2pDelay=20ms --errorRate=0.001 --transport_prot=ns3::TcpCubicFit --nWifi=6 --nFlows=2 --output_file=temp_fit_base.txt --ns3::TcpCubicFit::UseModifications=false" > temp_fit_base.log 2>&1
    metrics=$(extract_metrics temp_fit_base.log)
    echo "$r $metrics" >> Results/B_CUBIC_FIT/range_exp.txt

    # Modified CUBIC-FIT
    $CMD --command-template="%s --range=$r --p2pDelay=20ms --errorRate=0.001 --transport_prot=ns3::TcpCubicFit --nWifi=6 --nFlows=2 --output_file=temp_fit_mod.txt --ns3::TcpCubicFit::UseModifications=true" > temp_fit_mod.log 2>&1
    metrics=$(extract_metrics temp_fit_mod.log)
    echo "$r $metrics" >> Results/M_CUBIC_FIT/range_exp.txt
done

# 5. Nodes Experiment (Vary Nodes: 4 - 28)
echo "Running Nodes Experiment..."
rm -f Results/CUBIC/nodes_exp.txt Results/B_CUBIC_FIT/nodes_exp.txt Results/M_CUBIC_FIT/nodes_exp.txt
for n in 2 6 10 14; do
    # n is nodes-per-side. Total = n*2.
    total=$((n * 2))
    echo "  Nodes: $total"
    
    # Standard CUBIC
    $CMD --command-template="%s --nWifi=$n --p2pDelay=20ms --errorRate=0.001 --transport_prot=ns3::TcpCubic --nFlows=2 --output_file=temp_cubic.txt" > temp_cubic.log 2>&1
    metrics=$(extract_metrics temp_cubic.log)
    echo "$total $metrics" >> Results/CUBIC/nodes_exp.txt
    
    # Base CUBIC-FIT
    $CMD --command-template="%s --nWifi=$n --p2pDelay=20ms --errorRate=0.001 --transport_prot=ns3::TcpCubicFit --nFlows=2 --output_file=temp_fit_base.txt --ns3::TcpCubicFit::UseModifications=false" > temp_fit_base.log 2>&1
    metrics=$(extract_metrics temp_fit_base.log)
    echo "$total $metrics" >> Results/B_CUBIC_FIT/nodes_exp.txt

    # Modified CUBIC-FIT
    $CMD --command-template="%s --nWifi=$n --p2pDelay=20ms --errorRate=0.001 --transport_prot=ns3::TcpCubicFit --nFlows=2 --output_file=temp_fit_mod.txt --ns3::TcpCubicFit::UseModifications=true" > temp_fit_mod.log 2>&1
    metrics=$(extract_metrics temp_fit_mod.log)
    echo "$total $metrics" >> Results/M_CUBIC_FIT/nodes_exp.txt
done

echo "Experiments Complete."
