import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import os
import numpy as np

# Directories
BASE_DIR = "Results_Wired_Comp"
ALGORITHMS = {
    "CUBIC": {"dir": "CUBIC", "label": "TCP CUBIC", "color": "orange"},
    "B_CUBIC_FIT": {"dir": "B_CUBIC_FIT", "label": "Base CUBIC-FIT", "color": "blue"},
    "M_CUBIC_FIT": {"dir": "M_CUBIC_FIT", "label": "Modified CUBIC-FIT", "color": "green"}
}

OUT_DIR = "Results_Wired_Plots"
os.makedirs(OUT_DIR, exist_ok=True)

def read_data(algo, filename):
    filepath = os.path.join(BASE_DIR, ALGORITHMS[algo]["dir"], filename)
    x, thr, del_, pdr, drop, nrg = [], [], [], [], [], []
    if not os.path.exists(filepath):
        return x, thr, del_, pdr, drop, nrg
    with open(filepath, 'r') as f:
        for line in f:
            parts = line.split()
            if len(parts) >= 6:
                x.append(float(parts[0]))
                thr.append(float(parts[1]))
                del_.append(float(parts[2]))
                pdr.append(float(parts[3]))
                drop.append(float(parts[4]))
                nrg.append(float(parts[5]))
    return x, thr, del_, pdr, drop, nrg

def generate_comparisons(prefix, xlabel, filename):
    data = {}
    for algo in ALGORITHMS.keys():
        x, t, d, p, dr, n = read_data(algo, filename)
        if x:
            data[algo] = {"x": x, "thr": t, "del": d, "pdr": p, "drop": dr, "nrg": n}
    
    if "CUBIC" not in data or not data["CUBIC"]["x"]:
        return
        
    x_labels = data["CUBIC"]["x"]
    x_indexes = np.arange(len(x_labels))
    bar_width = 0.25
        
    def plot_metric(metric_key, ylabel, title, suffix):
        fig, ax = plt.subplots(figsize=(10, 6))
        
        ax.bar(x_indexes - bar_width, data['CUBIC'][metric_key], width=bar_width, label='TCP CUBIC', color=ALGORITHMS['CUBIC']['color'])
        if 'B_CUBIC_FIT' in data:
            ax.bar(x_indexes, data['B_CUBIC_FIT'][metric_key], width=bar_width, label='Base CUBIC-FIT', color=ALGORITHMS['B_CUBIC_FIT']['color'])
        if 'M_CUBIC_FIT' in data:
            ax.bar(x_indexes + bar_width, data['M_CUBIC_FIT'][metric_key], width=bar_width, label='Modified CUBIC-FIT', color=ALGORITHMS['M_CUBIC_FIT']['color'])
        
        ax.set_xlabel(xlabel)
        ax.set_ylabel(ylabel)
        ax.set_title(title)
        ax.set_xticks(x_indexes)
        ax.set_xticklabels(x_labels)
        ax.legend(loc='best')
        ax.grid(True, axis='y', linestyle='--', alpha=0.7)
        plt.tight_layout()
        plt.savefig(os.path.join(OUT_DIR, f"{prefix}_{suffix}.png"), dpi=300)
        plt.close()

    plot_metric("thr", 'Network Throughput (Kbps)', f'Throughput vs {xlabel}', 'throughput')
    plot_metric("del", 'End-to-End Delay (ns)', f'Delay vs {xlabel}', 'delay')
    plot_metric("pdr", 'Packet Delivery Ratio (%)', f'PDR vs {xlabel}', 'pdr')
    plot_metric("drop", 'Packet Drop Ratio (%)', f'Drop Ratio vs {xlabel}', 'drop')

if __name__ == "__main__":
    print("Generating Wired Comparison Bar Plots...")
    generate_comparisons("nodes", "Number of Nodes", "nodes.txt")
    generate_comparisons("flows", "Number of Flows", "flows.txt")
    generate_comparisons("pps", "Packets Per Second", "pps.txt")
    generate_comparisons("delay", "P2P Delay (ms)", "delay.txt")
    generate_comparisons("error", "Error Rate", "error.txt")
    print(f"All metric comparison plots generated in {OUT_DIR}/")
