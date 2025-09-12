#!/usr/bin/env python3
import csv
import sys

def calculate_speedups(csv_file):
    results = []
    
    with open(csv_file, 'r') as f:
        reader = csv.DictReader(f)
        for row in reader:
            kernel = row['kernel']
            baseline = float(row['baseline'])
            paralyze_4t = float(row['paralyze_4t'])
            gcc_auto = float(row['gcc_auto'])
            
            speedup = baseline / paralyze_4t
            gcc_speedup = baseline / gcc_auto
            efficiency = speedup / 4  # 4 threads
            
            results.append({
                'kernel': kernel,
                'baseline': baseline,
                'paralyze_4t': paralyze_4t,
                'speedup': speedup,
                'efficiency': efficiency,
                'gcc_speedup': gcc_speedup
            })
    
    # print summary
    print(f"{'Kernel':<12} {'Baseline':<10} {'PARALYZE 4t':<10} {'Speedup':<10} {'Efficiency':<12} {'GCC Speedup':<12}")
    print("-" * 76)
    
    total_speedup = 0
    for r in results:
        print(f"{r['kernel']:<12} {r['baseline']:<10.4f} {r['paralyze_4t']:<10.4f} {r['speedup']:<10.2f}x {r['efficiency']:<12.2%} {r['gcc_speedup']:<12.2f}x")
        total_speedup += r['speedup']
    
    avg_speedup = total_speedup / len(results)
    print("-" * 76)
    print(f"Average PARALYZE speedup: {avg_speedup:.2f}x")
    
    # best/worst
    best = max(results, key=lambda x: x['speedup'])
    worst = min(results, key=lambda x: x['speedup'])
    print(f"\nBest: {best['kernel']} ({best['speedup']:.2f}x)")
    print(f"Worst: {worst['kernel']} ({worst['speedup']:.2f}x)")

if __name__ == '__main__':
    calculate_speedups('../results/results.csv')