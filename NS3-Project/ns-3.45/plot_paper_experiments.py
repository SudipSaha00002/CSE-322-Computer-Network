import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt
import os

def read_data(filepath):
    t, c1, fit, c2 = [], [], [], []
    if not os.path.exists(filepath): return t, c1, fit, c2
    
    with open(filepath, 'r') as f:
        for line in f:
            parts = line.split()
            if len(parts) >= 4:
                t.append(float(parts[0]))
                c1.append(float(parts[1]))
                fit.append(float(parts[2]))
                c2.append(float(parts[3]))
    return t, c1, fit, c2

def moving_average(data, window_size=4):
    if not data: return []
    smoothed = []
    for i in range(len(data)):
        start = max(0, i - window_size + 1)
        window = data[start:i+1]
        smoothed.append(sum(window) / len(window))
    return smoothed

def plot_figure(data_file, output_file, title):
    t, c1, fit, c2 = read_data(data_file)
    if not t: return
    
    # Smooth TCP burst noise to match exact paper visual style
    c1_smooth = moving_average(c1, 5)
    c2_smooth = moving_average(c2, 5)
    fit_smooth = moving_average(fit, 5)

    plt.figure(figsize=(10, 6))
    
    plt.plot(t, c1_smooth, linestyle='-',  color='#ff7f0e', label='CUBIC 1', linewidth=2)
    plt.plot(t, c2_smooth, linestyle='-.', color='#2ca02c', label='CUBIC 2', linewidth=2)
    plt.plot(t, fit_smooth, linestyle='--', color='#1f77b4', label='CUBIC-FIT', linewidth=2)

    plt.title(title, y=-0.15)
    plt.xlabel('Time (Seconds)')
    plt.ylabel('Throughput (Mbps)')
    
    plt.xlim(0, 120)
    plt.ylim(0, max(10.0, max(c1 + fit + c2) + 1.0))
    
    plt.grid(True, linestyle=':', alpha=0.6)
    plt.legend(loc='upper right', frameon=True)
    
    plt.tight_layout(pad=2.0)
    plt.savefig(output_file, dpi=300)
    plt.close()

if __name__ == "__main__":
    os.makedirs("Results_Fig1", exist_ok=True)
    plot_figure("Results_Fig1/fig1_a_raw.txt", "Results_Fig1/fig1_a.png", "(a) Packet loss rate = 0%")
    plot_figure("Results_Fig1/fig1_b_raw.txt", "Results_Fig1/fig1_b.png", "(b) Packet loss rate = 1%")
