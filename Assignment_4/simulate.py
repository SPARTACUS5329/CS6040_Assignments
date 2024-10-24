import os
import subprocess
import matplotlib.pyplot as plt

def run_wfq(inputfile, outputfile):
    command = f"./wfq -in {inputfile} -out {outputfile}"
    try:
        subprocess.run(command, shell=True, check=True)
    except subprocess.CalledProcessError as e:
        print(f"Error running command: {e}")

def read_and_normalize_output(outputfile):
    with open(outputfile, 'r') as file:
        lines = file.readlines()

    data = []
    total_link_fraction = 0

    for line in lines:
        parts = line.split()
        connID = int(parts[0])
        linkFraction = float(parts[4])
        total_link_fraction += linkFraction
        data.append((connID, linkFraction))

    normalized_data = [(connID, linkFraction / total_link_fraction) for connID, linkFraction in data]
    
    return normalized_data

def plot_link_fraction_pie(normalized_data, title):
    connIDs = [f"Conn {d[0]}" for d in normalized_data]
    fractions = [d[1] for d in normalized_data]
    
    plt.figure(figsize=(6, 6))
    plt.pie(fractions, labels=connIDs, autopct='%1.1f%%')
    plt.title(title)
    plt.show()

def read_and_normalize_weights(inputfile):
    with open(inputfile, 'r') as file:
        lines = file.readlines()[1:]

    weights = []
    total_weight = 0

    for line in lines:
        parts = line.split()
        weight = float(parts[3])
        total_weight += weight
        weights.append(weight)

    normalized_weights = [weight / total_weight for weight in weights]

    return normalized_weights

def plot_weights_pie(normalized_weights, title):
    labels = [f"Flow {i+1}" for i in range(len(normalized_weights))]
    
    plt.figure(figsize=(6, 6))
    plt.pie(normalized_weights, labels=labels, autopct='%1.1f%%')
    plt.title(title)
    plt.show()

def weighted_jains_fairness_index(weights, link_utilizations):
    numerator = (sum(w * u for w, u in zip(weights, link_utilizations))) ** 2
    denominator = sum(weights) * sum(w * u ** 2 for w, u in zip(weights, link_utilizations))
    return numerator / denominator

if __name__ == "__main__":
    # run_wfq("inputfile8", "outputfile8")
    # run_wfq("inputfile16", "outputfile16")

    normalized_data8 = read_and_normalize_output("outputfile8")
    # plot_link_fraction_pie(normalized_data8, "Normalized Link Fraction (N=8)")

    normalized_data16 = read_and_normalize_output("outputfile16")
    # plot_link_fraction_pie(normalized_data16, "Normalized Link Fraction (N=16)")

    normalized_weights8 = read_and_normalize_weights("inputfile8")
    # plot_weights_pie(normalized_weights8, "Normalized Weights (N=8)")

    normalized_weights16 = read_and_normalize_weights("inputfile16")
    # plot_weights_pie(normalized_weights16, "Normalized Weights (N=16)")

    normalized_data8 = [i[1] for i in normalized_data8]
    normalized_data16 = [i[1] for i in normalized_data16]

    jainsFairness8 = weighted_jains_fairness_index(normalized_weights8, normalized_data8)
    jainsFairness16 = weighted_jains_fairness_index(normalized_weights16, normalized_data16)
    print(f"N = 8 -> fairness = {jainsFairness8}");
    print(f"N = 16 -> fairness = {jainsFairness16}");
