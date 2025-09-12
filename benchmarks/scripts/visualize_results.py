#!/usr/bin/env python3
import csv
import matplotlib.pyplot as plt
import numpy as np

def visualize_speedups(csv_file):
    kernels = []
    paralyze_speedups = []
    gcc_speedups = []
    
    with open(csv_file, 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            baseline = float(row['baseline'])
            paralyze_4t = float(row['paralyze_4t'])
            gcc_auto = float(row['gcc_auto'])
            
            kernels.append(row['kernel'])
            paralyze_speedups.append(baseline / paralyze_4t)
            gcc_speedups.append(baseline / gcc_auto)
    
    # create bar chart
    x = np.arange(len(kernels))
    width = 0.35
    
    fig, ax = plt.subplots(figsize=(14, 6))
    bars1 = ax.bar(x - width/2, paralyze_speedups, width, label='PARALYZE (4 threads)', color='#2ecc71')
    bars2 = ax.bar(x + width/2, gcc_speedups, width, label='GCC Auto-parallel', color='#e74c3c')
    
    # add baseline reference line
    ax.axhline(y=1.0, color='gray', linestyle='--', linewidth=1, label='Baseline (1.0x)')
    
    ax.set_xlabel('Kernel')
    ax.set_ylabel('Speedup (vs baseline)')
    ax.set_title('Parallelization Speedup Comparison: PARALYZE vs GCC Auto-parallel')
    ax.set_xticks(x)
    ax.set_xticklabels(kernels, rotation=45, ha='right')
    ax.legend()
    ax.grid(axis='y', alpha=0.3)
    
    plt.tight_layout()
    plt.savefig('../results/speedup_comparison.png', dpi=150, bbox_inches='tight')
    print("Saved speedup comparison chart to ../results/speedup_comparison.png")
    
    # create thread scaling chart for PARALYZE
    fig2, ax2 = plt.subplots(figsize=(14, 6))
    
    # get top 5 best performing kernels
    speedup_data = []
    with open(csv_file, 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            baseline = float(row['baseline'])
            speedup_4t = baseline / float(row['paralyze_4t'])
            speedup_data.append((row['kernel'], speedup_4t))
    
    speedup_data.sort(key=lambda x: x[1], reverse=True)
    top_kernels = [k for k, _ in speedup_data[:5]]
    
    # plot scaling for top kernels
    with open(csv_file, 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            if row['kernel'] in top_kernels:
                baseline = float(row['baseline'])
                threads = [1, 2, 4, 8]
                speedups = [
                    baseline / float(row['paralyze_1t']),
                    baseline / float(row['paralyze_2t']),
                    baseline / float(row['paralyze_4t']),
                    baseline / float(row['paralyze_8t'])
                ]
                ax2.plot(threads, speedups, marker='o', label=row['kernel'], linewidth=2)
    
    ax2.plot([1, 2, 4, 8], [1, 2, 4, 8], 'k--', alpha=0.3, label='Ideal scaling')
    
    ax2.set_xlabel('Number of Threads')
    ax2.set_ylabel('Speedup (vs baseline)')
    ax2.set_title('Thread Scaling for Top 5 Performing Kernels')
    ax2.legend()
    ax2.grid(alpha=0.3)
    ax2.set_xticks([1, 2, 4, 8])
    
    plt.tight_layout()
    plt.savefig('../results/thread_scaling.png', dpi=150, bbox_inches='tight')
    print("Saved thread scaling chart to ../results/thread_scaling.png")

if __name__ == '__main__':
    visualize_speedups('../results/results.csv')