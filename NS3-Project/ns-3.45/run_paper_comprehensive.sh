#!/bin/bash

# Comprehensive Evaluation Script for Paper Topology
CMD="./ns3 run scratch/cubic-fit-paper-topology"
export NS_LOG=""

# Output Directories
mkdir -p Results_Paper_Comp/CUBIC
mkdir -p Results_Paper_Comp/B_CUBIC_FIT
mkdir -p Results_Paper_Comp/M_CUBIC_FIT
mkdir -p Results_Paper_Comp/Comparison

extract_metrics() {
    local log_file=$1
    local thr=$(grep "Average Throughput:" $log_file | awk '{print $3}')
    local del=$(grep "Average E2E Delay:" $log_file | grep -o "[0-9.]*")
    local pdr=$(grep "Packet Delivery Ratio:" $log_file | awk '{print $4}' | tr -d '%')
    local drop=$(grep "Packet Drop Ratio:" $log_file | awk '{print $4}' | tr -d '%')
    local fair=$(grep "Jain's Fairness Index:" $log_file | grep -v "nan" | awk '{print $4}')
    
    if [ -z "$thr" ]; then thr="0"; fi
    if [ -z "$del" ]; then del="0"; fi
    if [ -z "$pdr" ]; then pdr="0"; fi
    if [ -z "$drop" ]; then drop="0"; fi
    if [ -z "$fair" ]; then fair="0"; fi
    
    local del_ms=$(echo $del | tr -d 'ms')
    echo "$thr $del_ms $pdr $drop $fair"
}

# 1. Delay Experiment
echo "Running Delay Experiment..."
rm -f Results_Paper_Comp/CUBIC/delay_exp.txt Results_Paper_Comp/B_CUBIC_FIT/delay_exp.txt Results_Paper_Comp/M_CUBIC_FIT/delay_exp.txt
for d in 10 50 100 150 200; do
    delay="${d}ms"
    echo "  Delay: $delay"
    
    # CUBIC
    $CMD --command-template="%s --delay=$delay --bw=10Mbps --errorRate=0.000 --nFit=0 --nCubic=2 --nUdp=0 --output_file=temp_c.txt" > temp_c.log 2>&1
    echo "$d $(extract_metrics temp_c.log)" >> Results_Paper_Comp/CUBIC/delay_exp.txt
    
    # Base FIT
    $CMD --command-template="%s --delay=$delay --bw=10Mbps --errorRate=0.000 --nFit=2 --nCubic=0 --nUdp=0 --useMod=false --output_file=temp_f1.txt" > temp_f1.log 2>&1
    echo "$d $(extract_metrics temp_f1.log)" >> Results_Paper_Comp/B_CUBIC_FIT/delay_exp.txt

    # Mod FIT
    $CMD --command-template="%s --delay=$delay --bw=10Mbps --errorRate=0.000 --nFit=2 --nCubic=0 --nUdp=0 --useMod=true --output_file=temp_f2.txt" > temp_f2.log 2>&1
    echo "$d $(extract_metrics temp_f2.log)" >> Results_Paper_Comp/M_CUBIC_FIT/delay_exp.txt
done

# 2. PLR Experiment
echo "Running PLR Experiment..."
rm -f Results_Paper_Comp/CUBIC/plr_exp.txt Results_Paper_Comp/B_CUBIC_FIT/plr_exp.txt Results_Paper_Comp/M_CUBIC_FIT/plr_exp.txt
for p in 0.000 0.005 0.010 0.015 0.020; do
    echo "  PLR: $p"
    
    # CUBIC
    $CMD --command-template="%s --delay=20ms --bw=10Mbps --errorRate=$p --nFit=0 --nCubic=2 --nUdp=0 --output_file=temp_c.txt" > temp_c.log 2>&1
    echo "$p $(extract_metrics temp_c.log)" >> Results_Paper_Comp/CUBIC/plr_exp.txt
    
    # Base FIT
    $CMD --command-template="%s --delay=20ms --bw=10Mbps --errorRate=$p --nFit=2 --nCubic=0 --nUdp=0 --useMod=false --output_file=temp_f1.txt" > temp_f1.log 2>&1
    echo "$p $(extract_metrics temp_f1.log)" >> Results_Paper_Comp/B_CUBIC_FIT/plr_exp.txt

    # Mod FIT
    $CMD --command-template="%s --delay=20ms --bw=10Mbps --errorRate=$p --nFit=2 --nCubic=0 --nUdp=0 --useMod=true --output_file=temp_f2.txt" > temp_f2.log 2>&1
    echo "$p $(extract_metrics temp_f2.log)" >> Results_Paper_Comp/M_CUBIC_FIT/plr_exp.txt
done

# 3. Flows Experiment
echo "Running Flows Experiment..."
rm -f Results_Paper_Comp/CUBIC/flows_exp.txt Results_Paper_Comp/B_CUBIC_FIT/flows_exp.txt Results_Paper_Comp/M_CUBIC_FIT/flows_exp.txt
for f in 2 4 6 8 10; do
    echo "  Flows: $f"
    
    # CUBIC
    $CMD --command-template="%s --delay=20ms --bw=10Mbps --errorRate=0.000 --nFit=0 --nCubic=$f --nUdp=0 --output_file=temp_c.txt" > temp_c.log 2>&1
    echo "$f $(extract_metrics temp_c.log)" >> Results_Paper_Comp/CUBIC/flows_exp.txt
    
    # Base FIT
    $CMD --command-template="%s --delay=20ms --bw=10Mbps --errorRate=0.000 --nFit=$f --nCubic=0 --nUdp=0 --useMod=false --output_file=temp_f1.txt" > temp_f1.log 2>&1
    echo "$f $(extract_metrics temp_f1.log)" >> Results_Paper_Comp/B_CUBIC_FIT/flows_exp.txt

    # Mod FIT
    $CMD --command-template="%s --delay=20ms --bw=10Mbps --errorRate=0.000 --nFit=$f --nCubic=0 --nUdp=0 --useMod=true --output_file=temp_f2.txt" > temp_f2.log 2>&1
    echo "$f $(extract_metrics temp_f2.log)" >> Results_Paper_Comp/M_CUBIC_FIT/flows_exp.txt
done

# 4. Nodes Experiment (In dumbbell, synonymous with flows, but mapped to higher scales to match previous test)
echo "Running Nodes Experiment..."
rm -f Results_Paper_Comp/CUBIC/nodes_exp.txt Results_Paper_Comp/B_CUBIC_FIT/nodes_exp.txt Results_Paper_Comp/M_CUBIC_FIT/nodes_exp.txt
for n in 4 12 20 28 36; do
    echo "  Nodes: $n"
    # Equivalent to n flows
    
    # CUBIC
    $CMD --command-template="%s --delay=20ms --bw=10Mbps --errorRate=0.000 --nFit=0 --nCubic=$n --nUdp=0 --output_file=temp_c.txt" > temp_c.log 2>&1
    echo "$n $(extract_metrics temp_c.log)" >> Results_Paper_Comp/CUBIC/nodes_exp.txt
    
    # Base FIT
    $CMD --command-template="%s --delay=20ms --bw=10Mbps --errorRate=0.000 --nFit=$n --nCubic=0 --nUdp=0 --useMod=false --output_file=temp_f1.txt" > temp_f1.log 2>&1
    echo "$n $(extract_metrics temp_f1.log)" >> Results_Paper_Comp/B_CUBIC_FIT/nodes_exp.txt

    # Mod FIT
    $CMD --command-template="%s --delay=20ms --bw=10Mbps --errorRate=0.000 --nFit=$n --nCubic=0 --nUdp=0 --useMod=true --output_file=temp_f2.txt" > temp_f2.log 2>&1
    echo "$n $(extract_metrics temp_f2.log)" >> Results_Paper_Comp/M_CUBIC_FIT/nodes_exp.txt
done

echo "Experiments Complete."
rm -f temp_c.* temp_f1.* temp_f2.*
