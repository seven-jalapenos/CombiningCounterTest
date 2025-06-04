import pandas as pd
import matplotlib.pyplot as plt

def plot_both(df: pd.DataFrame, xaxis, plot_x, title, filename) -> None:
    plt.figure(figsize=(8, 5))
    plt.plot(df[plot_x], df["combining"], marker='o', label="Combining Tree")
    plt.plot(df[plot_x], df["sequential"], marker='s', label="One-Lock")
    plt.xlabel(xaxis)
    plt.ylabel("Microseconds")
    plt.title(title)
    plt.grid(True)
    plt.legend()
    plt.tight_layout()
    plt.savefig(filename)

