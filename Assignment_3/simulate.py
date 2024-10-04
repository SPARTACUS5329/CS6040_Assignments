import subprocess
import os
import numpy as np
import matplotlib.pyplot as plt
from math import ceil

q_values = ['INQ', 'NOQ', 'CIOQ']
N_values = [8, 16, 32, 64, 128]
p_values = [0.4, 0.6, 0.8, 1.0]
lk_scaling_factors = [0.4, 0.7, 1.0]

B = 20
T = 10000

output_base_dir = "outputs"
os.makedirs(output_base_dir, exist_ok=True)

def generate_heatmap(data, x_labels, y_labels, title, xlabel, ylabel):
    fig, ax = plt.subplots()
    cax = ax.imshow(data, cmap="YlGnBu", aspect='auto')
    cbar = fig.colorbar(cax)
    ax.set_xticks(np.arange(len(x_labels)))
    ax.set_yticks(np.arange(len(y_labels)))
    ax.set_xticklabels(x_labels)
    ax.set_yticklabels(y_labels)
    ax.set_xlabel(xlabel)
    ax.set_ylabel(ylabel)
    ax.set_title(title)
    plt.show()


def run_program(N, B, p, q, L, K, T, output_file):
    command = f"./routing -N {N} -B {B} -p {p} -q {q} -L {L} -K {K} -o {output_file} -T {T}"
    subprocess.run(command, shell=True)

def parse_output(output_file, q):
    with open(output_file, 'r') as f:
        line = f.readline().strip()
        data = line.split('\t')
        if q == 'CIOQ':
            avg_packet_delay = float(data[5])
            avg_port_utilization = float(data[6])
        else:
            avg_packet_delay = float(data[3])
            avg_port_utilization = float(data[4])
        return avg_packet_delay, avg_port_utilization

for q in q_values:
    q_output_dir = os.path.join(output_base_dir, q)
    os.makedirs(q_output_dir, exist_ok=True)
    
    heatmap_data_delay = []
    heatmap_data_utilization = []
    
    for N in N_values:
        row_delay = []
        row_utilization = []
        for p in p_values:
            if q == 'CIOQ':
                total_delay = 0
                total_utilization = 0
                count = 0
                for l_factor in lk_scaling_factors:
                    L = ceil(l_factor * N)
                    K = ceil(l_factor * N)
                    output_file = os.path.join(q_output_dir, f"output_{q}_{N}_{p}_L{L}_K{K}.txt")
                    run_program(N, B, p, q, L, K, T, output_file)
                    avg_packet_delay, avg_port_utilization = parse_output(output_file, q)
                    total_delay += avg_packet_delay
                    total_utilization += avg_port_utilization
                    count += 1
                
                avg_delay = total_delay / count
                avg_utilization = total_utilization / count
                
                row_delay.append(avg_delay)
                row_utilization.append(avg_utilization)
            else:
                output_file = os.path.join(q_output_dir, f"output_{q}_{N}_{p}.txt")
                run_program(N, B, p, q, 1, 1, T, output_file)
                avg_packet_delay, avg_port_utilization = parse_output(output_file, q)
                row_delay.append(avg_packet_delay)
                row_utilization.append(avg_port_utilization)
        
        heatmap_data_delay.append(row_delay)
        heatmap_data_utilization.append(row_utilization)

    heatmap_data_delay = np.array(heatmap_data_delay)
    heatmap_data_utilization = np.array(heatmap_data_utilization)

    generate_heatmap(heatmap_data_delay, p_values, N_values, f"Heatmap for avgPacketDelay (q={q})", "p values", "N values");

    generate_heatmap(heatmap_data_utilization, p_values, N_values, f"Heatmap for avgPortUtilization (q={q})", "p values", "N values");

for N in [32, 64]:
    for p in [0.4, 1.0]:
        q = 'CIOQ'
        heatmap_data_delay = []
        heatmap_data_utilization = []
        L_values = []
        K_values = []
        
        for l_factor in lk_scaling_factors:
            row_delay = []
            row_utilization = []
            L = ceil(l_factor * N)
            L_values.append(L)
            K_values.clear()
            for k_factor in lk_scaling_factors:
                K = ceil(k_factor * N)
                K_values.append(K)
                output_file = os.path.join(output_base_dir, f"output_{q}_{N}_{p}_L{L}_K{K}.txt")
                run_program(N, B, p, q, L, K, T, output_file)
                avg_packet_delay, avg_port_utilization = parse_output(output_file, q)
                
                row_delay.append(avg_packet_delay)
                row_utilization.append(avg_port_utilization)

            heatmap_data_delay.append(row_delay)
            heatmap_data_utilization.append(row_utilization)

        heatmap_data_delay = np.array(heatmap_data_delay)
        heatmap_data_utilization = np.array(heatmap_data_utilization)
        
        generate_heatmap(heatmap_data_delay, L_values, K_values, 
                         f"avgPacketDelay Heatmap for CIOQ (N={N}, p={p})", "L values", "K values")

        generate_heatmap(heatmap_data_utilization, L_values, K_values, 
                         f"avgPortUtilization Heatmap for CIOQ (N={N}, p={p})", "L values", "K values")
