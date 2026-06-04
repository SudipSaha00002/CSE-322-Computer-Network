import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import os
import sys

# Directories
BEFORE_DIR = "Results_Paper_Comp/CUBIC"
BASE_DIR = "Results_Paper_Comp/B_CUBIC_FIT"
AFTER_DIR = "Results_Paper_Comp/M_CUBIC_FIT"
COMP_DIR = "Results_Paper_Comp/Comparison"
TRACE_DIR = "Results_Paper_Comp/Traces"

# Ensure directories exist
os.makedirs(BEFORE_DIR, exist_ok=True)
os.makedirs(BASE_DIR, exist_ok=True)
os.makedirs(AFTER_DIR, exist_ok=True)
os.makedirs(COMP_DIR, exist_ok=True)

def read_data(filepath):
    x = []
    y_thr = []
    y_del = []
    y_pdr = []
    y_drop = []
    y_fair = []
    
    if not os.path.exists(filepath):
        print(f"Warning: File {filepath} not found.")
        return x, y_thr, y_del, y_pdr, y_drop, y_fair

    with open(filepath, 'r') as f:
        for line in f:
            parts = line.split()
            if len(parts) < 6:
                continue
            x.append(float(parts[0].replace('ms', ''))) # Handle '20ms' if needed
            y_thr.append(float(parts[1]))
            y_del.append(float(parts[2]))
            y_pdr.append(float(parts[3]))
            y_drop.append(float(parts[4]))
            y_fair.append(float(parts[5]))
    return x, y_thr, y_del, y_pdr, y_drop, y_fair

import numpy as np

def plot_metric(filename, x_label, plot_name_suffix):
    # Read Before
    xb, yb_thr, yb_del, yb_pdr, yb_drop, yb_fair = read_data(os.path.join(BEFORE_DIR, filename))
    # Read Base (Original CUBIC-FIT)
    xbase, ybase_thr, ybase_del, ybase_pdr, ybase_drop, ybase_fair = read_data(os.path.join(BASE_DIR, filename))
    # Read After (Modified CUBIC-FIT)
    xa, ya_thr, ya_del, ya_pdr, ya_drop, ya_fair = read_data(os.path.join(AFTER_DIR, filename))

    # Metrics configuration
    metrics = [
        (yb_thr, ybase_thr, ya_thr, "Throughput (Mbps)", "throughput"),
        (yb_del, ybase_del, ya_del, "End-to-End Delay (ms)", "delay"),
        (yb_pdr, ybase_pdr, ya_pdr, "Packet Delivery Ratio (%)", "pdr"),
        (yb_drop, ybase_drop, ya_drop, "Packet Drop Ratio (%)", "drop"),
        (yb_fair, ybase_fair, ya_fair, "Jain's Fairness Index", "fairness")
    ]

    for yb, ybase, ya, ylabel, name in metrics:
        if not yb and not ybase and not ya:
            continue
            
        # Determine X-axis labels and indices
        x_values = xa if xa else (xbase if xbase else xb)
        if not x_values:
            continue
            
        indices = np.arange(len(x_values))
        width3 = 0.25  # width for 3 bars
        width2 = 0.35  # width for 2 bars

        # --- 1. Plot 3-Way Comparison (Comparison Folder) ---
        fig, ax = plt.subplots(figsize=(10, 6))
        if yb:
            min_len = min(len(indices), len(yb))
            rects1 = ax.bar(indices[:min_len] - width3, yb[:min_len], width3, label='Standard CUBIC', color='skyblue')
            ax.bar_label(rects1, padding=3, fmt='%.2f', fontsize=8)
        if ybase:
            min_len = min(len(indices), len(ybase))
            rects2 = ax.bar(indices[:min_len], ybase[:min_len], width3, label='Base CUBIC-FIT', color='lightgreen')
            ax.bar_label(rects2, padding=3, fmt='%.2f', fontsize=8)
        if ya:
            min_len = min(len(indices), len(ya))
            rects3 = ax.bar(indices[:min_len] + width3, ya[:min_len], width3, label='Modified CUBIC-FIT', color='salmon')
            ax.bar_label(rects3, padding=3, fmt='%.2f', fontsize=8)

        ax.set_xlabel(x_label)
        ax.set_ylabel(ylabel)
        ax.set_title(f"{ylabel} vs {x_label} (3-Way Comparison)")
        ax.set_xticks(indices)
        ax.set_xticklabels(x_values)
        ax.legend()
        ax.grid(True, axis='y', linestyle='--', alpha=0.7)
        fig.tight_layout()
        plt.savefig(os.path.join(COMP_DIR, f"{name}_{plot_name_suffix}.png"))
        plt.close()

        # --- 2. Plot 2-Way Comparison (Standard vs Modified) (M_CUBIC_FIT Folder) ---
        fig, ax = plt.subplots(figsize=(10, 6))
        if yb:
            min_len = min(len(indices), len(yb))
            rects1 = ax.bar(indices[:min_len] - width2/2, yb[:min_len], width2, label='Standard CUBIC', color='skyblue')
            ax.bar_label(rects1, padding=3, fmt='%.2f', fontsize=8)
        if ya:
            min_len = min(len(indices), len(ya))
            rects2 = ax.bar(indices[:min_len] + width2/2, ya[:min_len], width2, label='Modified CUBIC-FIT', color='salmon')
            ax.bar_label(rects2, padding=3, fmt='%.2f', fontsize=8)
        
        ax.set_xlabel(x_label)
        ax.set_ylabel(ylabel)
        ax.set_title(f"{ylabel} vs {x_label} (Standard vs Modified)")
        ax.set_xticks(indices)
        ax.set_xticklabels(x_values)
        ax.legend()
        ax.grid(True, axis='y', linestyle='--', alpha=0.7)
        fig.tight_layout()
        plt.savefig(os.path.join(AFTER_DIR, f"{name}_{plot_name_suffix}.png"))
        plt.close()
        
        # --- 3. Plot 2-Way Comparison (Standard vs Base) (B_CUBIC_FIT Folder) ---
        fig, ax = plt.subplots(figsize=(10, 6))
        if yb:
            min_len = min(len(indices), len(yb))
            rects1 = ax.bar(indices[:min_len] - width2/2, yb[:min_len], width2, label='Standard CUBIC', color='skyblue')
            ax.bar_label(rects1, padding=3, fmt='%.2f', fontsize=8)
        if ybase:
            min_len = min(len(indices), len(ybase))
            rects2 = ax.bar(indices[:min_len] + width2/2, ybase[:min_len], width2, label='Base CUBIC-FIT', color='lightgreen')
            ax.bar_label(rects2, padding=3, fmt='%.2f', fontsize=8)
        
        ax.set_xlabel(x_label)
        ax.set_ylabel(ylabel)
        ax.set_title(f"{ylabel} vs {x_label} (Standard vs Base)")
        ax.set_xticks(indices)
        ax.set_xticklabels(x_values)
        ax.legend()
        ax.grid(True, axis='y', linestyle='--', alpha=0.7)
        fig.tight_layout()
        plt.savefig(os.path.join(BASE_DIR, f"{name}_{plot_name_suffix}.png"))
        plt.close()

        # --- 4. Plot Baseline (CUBIC Folder) ---
        if yb:
            fig, ax = plt.subplots(figsize=(10, 6))
            rects1 = ax.bar(indices[:len(yb)], yb, width2, label='Standard CUBIC', color='skyblue')
            ax.bar_label(rects1, padding=3, fmt='%.2f')
            ax.set_xlabel(x_label)
            ax.set_ylabel(ylabel)
            ax.set_title(f"{ylabel} vs {x_label} (Baseline)")
            ax.set_xticks(indices)
            ax.set_xticklabels(x_values)
            ax.legend()
            ax.grid(True, axis='y', linestyle='--', alpha=0.7)
            fig.tight_layout()
            plt.savefig(os.path.join(BEFORE_DIR, f"{name}_{plot_name_suffix}.png"))
            plt.close()

def read_trace(filepath):
    t = []
    v = []
    if not os.path.exists(filepath):
        return t, v
    with open(filepath, 'r') as f:
        for line in f:
            parts = line.split()
            if len(parts) >= 2:
                t.append(float(parts[0]))
                v.append(float(parts[1]))
    return t, v

def plot_time_series():
    # Alpha
    t, v = read_trace(os.path.join(TRACE_DIR, "trace_fit_alpha.dat"))
    if t:
        plt.figure()
        plt.plot(t, v, 'g-', label="Alpha")
        plt.xlabel("Time (s)")
        plt.ylabel("Alpha Value")
        plt.title("Dynamic Alpha Adaptation over Time")
        plt.legend()
        plt.grid(True)
        plt.savefig(os.path.join(AFTER_DIR, "alpha_vs_time.png"))
        plt.close()

    # N Flows
    t, v = read_trace(os.path.join(TRACE_DIR, "trace_fit_n.dat"))
    if t:
        plt.figure()
        plt.plot(t, v, 'm-', label="Virtual N")
        plt.xlabel("Time (s)")
        plt.ylabel("N (Virtual Flows)")
        plt.title("Virtual Flow Scaling (N) over Time")
        plt.legend()
        plt.grid(True)
        plt.savefig(os.path.join(AFTER_DIR, "n_vs_time.png"))
        plt.close()
    
    # Queue Delay
    t, v = read_trace(os.path.join(TRACE_DIR, "trace_fit_queue.dat"))
    if t:
        plt.figure()
        plt.plot(t, v, 'c-', label="Queue Delay")
        plt.xlabel("Time (s)")
        plt.ylabel("Queue Delay (s)")
        plt.title("Bottleneck Queue Estimation over Time")
        plt.legend()
        plt.grid(True)
        plt.savefig(os.path.join(AFTER_DIR, "queue_vs_time.png"))
        plt.close()

    # CWND Comparison
    tb, vb = read_trace(os.path.join(TRACE_DIR, "trace_cubic_cwnd.dat"))
    ta, va = read_trace(os.path.join(TRACE_DIR, "trace_fit_cwnd.dat"))
    
    if tb or ta:
        plt.figure()
        if tb:
            # Downsample for readability if needed, but let's plotting raw
            plt.plot(tb, vb, 'b-', alpha=0.5, label="CUBIC CWND")
        if ta:
            plt.plot(ta, va, 'r-', alpha=0.8, label="CUBIC-FIT CWND")
        plt.xlabel("Time (s)")
        plt.ylabel("Congestion Window (Bytes)")
        plt.title("CWND Evolution vs Time")
        plt.legend()
        plt.grid(True)
        plt.savefig(os.path.join(AFTER_DIR, "cwnd_vs_time.png"))
        plt.close()


if __name__ == "__main__":
    # 1. Delay Experiment
    plot_metric("delay_exp.txt", "P2P Delay (ms)", "vs_delay")
    
    # 2. PLR Experiment
    plot_metric("plr_exp.txt", "Packet Loss Rate", "vs_plr")
    
    # 3. Flows Experiment
    plot_metric("flows_exp.txt", "Number of Flows", "vs_flows")
    
    # 4. Range Experiment
    plot_metric("range_exp.txt", "Transmission Range (m)", "vs_range")
    
    # 5. Nodes Experiment
    plot_metric("nodes_exp.txt", "Total Number of Nodes", "vs_nodes")
    
    # 6. Time Series
    plot_time_series()
    
    print("Plots generated in Results_Paper_Comp/CUBIC and Results_Paper_Comp/Comparison")
