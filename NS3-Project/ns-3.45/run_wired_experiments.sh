#!/bin/bash
./ns3 build

mkdir -p Results_Wired_Comp
rm -rf Results_Wired_Comp/*

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

CMD="./ns3 run scratch/cubic-fit-wired"

ALGORITHMS=("CUBIC" "B_CUBIC_FIT" "M_CUBIC_FIT")

echo "=== Running Wired Simulations (3 Variants) ==="

for algo in "${ALGORITHMS[@]}"; do
    echo "Starting $algo..."
    mkdir -p Results_Wired_Comp/$algo
    
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
        $CMD --command-template="%s --nWifi=$n --nFlows=$n $ALGO_ARGS" > temp.log 2>&1
        extract_metrics $tnode temp.log >> Results_Wired_Comp/$algo/nodes.txt
    done

    # 2. Flow Sweep
    echo "  - Flow Sweep..."
    for f in 10 20 30 40 50; do
        $CMD --command-template="%s --nWifi=50 --nFlows=$f $ALGO_ARGS" > temp.log 2>&1
        extract_metrics $f temp.log >> Results_Wired_Comp/$algo/flows.txt
    done

    # 3. PPS Sweep
    echo "  - PPS Sweep..."
    for p in 100 200 300 400 500; do
        $CMD --command-template="%s --nWifi=10 --nFlows=10 --pps=$p $ALGO_ARGS" > temp.log 2>&1
        extract_metrics $p temp.log >> Results_Wired_Comp/$algo/pps.txt
    done

    # 4. Delay Sweep
    echo "  - Delay Sweep..."
    for d in 20 40 60 80 100; do
        $CMD --command-template="%s --nWifi=10 --nFlows=10 --p2pDelay=${d}ms $ALGO_ARGS" > temp.log 2>&1
        extract_metrics $d temp.log >> Results_Wired_Comp/$algo/delay.txt
    done

    # 5. Error Rate Sweep
    echo "  - Error Rate Sweep..."
    for e in 0.00 0.005 0.01 0.015 0.02; do
        $CMD --command-template="%s --nWifi=10 --nFlows=10 --errorRate=$e $ALGO_ARGS" > temp.log 2>&1
        extract_metrics $e temp.log >> Results_Wired_Comp/$algo/error.txt
    done
done

rm -f temp.log ignore.txt
echo "=== All wired simulations completed successfully! ==="
