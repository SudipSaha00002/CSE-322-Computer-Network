#!/bin/bash
./ns3 build

mkdir -p Results_Cubic_Comp
rm -rf Results_Cubic_Comp/*

extract_metrics() {
    local val=$1
    local log=$2
    local thr=$(grep "Total Throughput:" $log | awk '{print $3}')
    local del=$(grep "Avg End-to-End Delay:" $log | awk '{print $4}')
    local pdr=$(grep "Packet Delivery Ratio:" $log | awk '{print $4}')
    local drop=$(grep "Packet Drop Ratio:" $log | awk '{print $4}')
    local nrg=$(grep "Total Energy Consumed:" $log | awk '{print $4}')
    
    if [ -z "$thr" ]; then thr="0"; fi
    if [ -z "$del" ]; then del="0"; fi
    if [ -z "$pdr" ]; then pdr="0"; fi
    if [ -z "$drop" ]; then drop="0"; fi
    if [ -z "$nrg" ]; then nrg="0"; fi
    
    echo "$val $thr $del $pdr $drop $nrg"
}

CMD="./ns3 run scratch/cubic-fit-wireless"

ALGORITHMS=("CUBIC" "B_CUBIC_FIT" "M_CUBIC_FIT")

echo "=== Running Cubic Simulations (3 Variants) ==="

for algo in "${ALGORITHMS[@]}"; do
    echo "Starting $algo..."
    mkdir -p Results_Cubic_Comp/$algo
    
    if [ "$algo" == "CUBIC" ]; then
        ALGO_ARGS="--transport_prot=ns3::TcpCubic"
    elif [ "$algo" == "B_CUBIC_FIT" ]; then
        ALGO_ARGS="--transport_prot=ns3::TcpCubicFit --ns3::TcpCubicFit::UseModifications=false"
    else
        ALGO_ARGS="--transport_prot=ns3::TcpCubicFit --ns3::TcpCubicFit::UseModifications=true"
    fi

    # 1. Node Sweep
    echo "  - Node Sweep..."
    for n in 10 20 30 40 50; do
        tnode=$((n*2))
        $CMD --command-template="%s --nWifi=$n --nFlows=$n --isMobile=false $ALGO_ARGS" > temp.log 2>&1
        extract_metrics $tnode temp.log >> Results_Cubic_Comp/$algo/nodes.txt
    done

    # 2. Flow Sweep
    echo "  - Flow Sweep..."
    for f in 10 20 30 40 50; do
        $CMD --command-template="%s --nWifi=50 --nFlows=$f --isMobile=false $ALGO_ARGS" > temp.log 2>&1
        extract_metrics $f temp.log >> Results_Cubic_Comp/$algo/flows.txt
    done

    # 3. PPS Sweep
    echo "  - PPS Sweep..."
    for p in 100 200 300 400 500; do
        $CMD --command-template="%s --nWifi=10 --nFlows=10 --pps=$p --isMobile=false $ALGO_ARGS" > temp.log 2>&1
        extract_metrics $p temp.log >> Results_Cubic_Comp/$algo/pps.txt
    done

    # 4. Speed Sweep
    echo "  - Speed Sweep..."
    for s in 5 10 15 20 25; do
        $CMD --command-template="%s --nWifi=10 --nFlows=10 --isMobile=true --nodeSpeed=$s $ALGO_ARGS" > temp.log 2>&1
        extract_metrics $s temp.log >> Results_Cubic_Comp/$algo/speed.txt
    done

    # 5. Coverage Sweep
    echo "  - Coverage Area Sweep..."
    for c in 1 2 3 4 5; do
        $CMD --command-template="%s --nWifi=10 --nFlows=10 --isMobile=false --coverageScale=$c $ALGO_ARGS" > temp.log 2>&1
        extract_metrics $c temp.log >> Results_Cubic_Comp/$algo/coverage.txt
    done
done

rm -f temp.log ignore.txt
echo "=== All simulations completed successfully! ==="
