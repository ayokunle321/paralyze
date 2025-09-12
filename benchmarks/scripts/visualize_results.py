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

if __name__ == '__main__':
    visualize_speedups('../results/results.csv')