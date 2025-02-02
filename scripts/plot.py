import pandas as pd
import matplotlib.pyplot as plt
import seaborn as sns
import os

# Configure paths and style
results_dir = "../results/logs"
plots_dir = "../results/plots"
os.makedirs(plots_dir, exist_ok=True)  # Create directory if missing
sns.set(style="whitegrid", palette="tab10", context="talk")  # Better styling

def create_plots():
    for log_file in os.listdir(results_dir):
        if log_file.endswith("_results.csv"):
            exe_name = log_file.replace("_results.csv", "")
            file_path = os.path.join(results_dir, log_file)

            try:
                df = pd.read_csv(file_path)

                required_cols = {'threads', 'affinity', 'time'}
                if not required_cols.issubset(df.columns):
                    print(f"Skipping {log_file}: Missing required columns")
                    continue

                plt.figure(figsize=(12, 8))

                palette = sns.color_palette("husl", n_colors=len(df['affinity'].unique()))

                for i, affinity in enumerate(sorted(df['affinity'].unique(), reverse=True)):
                    subset = df[df['affinity'] == affinity]
                    plt.plot(subset['threads'], subset['time'],
                             marker='o', linestyle='--', linewidth=2.5,
                             markersize=10, label=f"Affinity={affinity}",
                             color=palette[i])

                plt.title(f"{exe_name} Performance\nThread Scaling", pad=20, fontsize=18, fontweight='bold')

                plt.xlabel("Number of Threads", labelpad=15, fontsize=14, fontweight='bold')
                plt.ylabel("Execution Time (s)", labelpad=15, fontsize=14, fontweight='bold')

                plt.xticks(df['threads'].unique(), fontsize=12)
                plt.yticks(fontsize=12)

                plt.grid(True, which='both', linestyle='--', linewidth=0.5, alpha=0.7)

                legend = plt.legend(title="Affinity Settings", fontsize=12, title_fontsize=13,
                                    bbox_to_anchor=(1.05, 1), loc='upper left', borderaxespad=0.)
                legend.get_title().set_fontweight('bold')

                min_time = df['time'].min()
                plt.annotate(f'Fastest: {min_time:.2f}s',
                             xy=(0.05, 0.95), xycoords='axes fraction',
                             fontsize=14, color='darkgreen', fontweight='bold',
                             bbox=dict(facecolor='white', alpha=0.8, edgecolor='darkgreen', boxstyle='round,pad=0.5'))

                # Save in multiple formats
                plot_path = os.path.join(plots_dir, exe_name)
                plt.savefig(f"{plot_path}_plot.png", dpi=300, bbox_inches='tight')
                plt.savefig(f"{plot_path}_plot.pdf", bbox_inches='tight')
                plt.close()

                print(f"Generated plot for {exe_name}")

            except Exception as e:
                print(f"Error processing {log_file}: {str(e)}")

if __name__ == "__main__":
    create_plots()