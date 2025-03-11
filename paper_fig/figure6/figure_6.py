""""
"MIT License

Copyright (c) 2025 Giuseppe Sorrentino, Paolo Salvatore Galfano, Davide Conficconi, Eleonora D'Arnese

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
"""


import os
import pandas as pd
import seaborn as sns
import matplotlib.pyplot as plt

plt.rcdefaults()

plt.rcParams.update({
    "text.usetex": True,
    "font.family": "serif",
    "font.serif": ["Palatino"],
})
plt.rcParams["font.size"] = 12
plt.rcParams["xtick.labelsize"] = 12  # Major tick size in points
plt.rcParams["ytick.labelsize"] = 12  # Major tick size in points
plt.rcParams["legend.fontsize"] = 12  # Legend font size
plt.rcParams["legend.handletextpad"] = 0.01  # Padding between legend handle and text

plt.rcParams['hatch.linewidth'] = 0.6
plt.rcParams['axes.labelpad'] = 0
plt.rcParams['pdf.fonttype'] = 42
plt.rcParams['ps.fonttype'] = 42
scale_points = 1.75


def process_csv_files(folder_path):
    # List to store final data
    results = []

    # Iterate through files in the folder
    for filename in os.listdir(folder_path):
        # Check if the file is a CSV
        if filename.endswith('.csv'):
            try:
                # Parse the filename
                parts = filename.split('_')
                if len(parts) != 4 or not parts[0] == 'time':
                    print(f"Invalid filename: {filename}")
                    continue

                IPE = parts[1].replace('IPE', '')
                Resolution = parts[2].replace('D', '')
                Depth = parts[3].replace('N', '').replace('.csv', '')

                # Read the CSV file
                file_path = os.path.join(folder_path, filename)
                data = pd.read_csv(file_path)

                if 'exec_time' not in data.columns:
                    print(f"Column 'exec_time' not found in {filename}")
                    continue

                # Calculate the average execution time
                average_time = data['exec_time'].mean()

                # Append results to the list
                results.append({
                    'IPE': IPE,
                    'Resolution': Resolution,
                    'Depth': Depth,
                    'Time': average_time
                })

            except Exception as e:
                print(f"Error processing {filename}: {e}")

    # Create a DataFrame from results
    results_df = pd.DataFrame(results)
    return results_df


def generate_plots(results_df, output_folder):
    # Ensure the output folder exists
    os.makedirs(output_folder, exist_ok=True)

    # Group by Resolution and create a plot for each
    for resolution, group in results_df.groupby('Resolution'):
        plt.figure(figsize=(10, 5))
        
        # Convert IPE and Depth to integers for correct sorting
        group['IPE'] = group['IPE'].astype(int)
        group['Depth'] = group['Depth'].astype(int)
        group = group.sort_values(by='IPE')

        # Use seaborn to create a line plot
        custom_palette = ["#762a83", "#af8dc3", "#e7d4e8", "#7fbf7b", "#1b7837"]

        # Set figure size
        plt.figure(figsize=(10, 4))
        sns.lineplot(data=group, x='IPE', y='Time', hue='Depth', marker='o', markersize=10, palette=custom_palette, linewidth=3)

        plt.xlabel("Employed IPEs", fontsize=20)
        plt.ylabel("Average Time [s]", fontsize=20)
        plt.xticks([1, 2, 4, 8, 16, 32], [1, 2, 4, 8, 16, 32], fontsize=18)

        # Increase tick label size
        plt.xticks(fontsize=18)
        plt.yticks(fontsize=18)
        plt.grid(False)

        # Increase space between legend color and label
        plt.legend(title="Depth", title_fontsize=18, fontsize=18, handletextpad=0.5)

        # Save the plot
        plt.savefig(os.path.join(output_folder, f"plot_resolution_{resolution}.pdf"), bbox_inches='tight')
        plt.close()


if __name__ == "__main__":
    folder_path = "./csv/"
    result_df = process_csv_files(folder_path)
    
    if result_df.empty:
        print("The results DataFrame is empty. Check the CSV files in the specified folder.")
        exit()


    # Save results to a CSV file
    result_df.to_csv(os.path.join(folder_path, 'final_results.csv'), index=False)

    # Generate plots
    print("Generating plots...")
    output_folder = "./plots"
    result_df = process_csv_files(folder_path)
    generate_plots(result_df, output_folder)
    print(f"Plots saved in folder: {output_folder}")
