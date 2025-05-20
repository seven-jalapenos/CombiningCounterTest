import subprocess
import csv
import sys

command = "./combining_tree_test" # (threads, contention, runs, op, backoff)
res_dir = "./results"

class Runtime:
    threads = 8
    contention = 0
    runs = 10
    op = 4
    backoff = 10
    
    def __str__(self):
        return f"threads: {self.threads}, contention: {self.contention}, op: {self.op}, backoff: {self.backoff}"

def benchmark(r: Runtime)->str:
    st = str(r.threads)
    sc = str(r.contention)
    sr = str(r.runs)
    so = str(r.op)
    sb = str(r.backoff)

    result = None
    try:
        result = subprocess.run(
            [command, st, sc, sr, so, sb],
            capture_output=True,
            text=True,
            check=True
        )
    except Exception as e:
        print(f"""run with parameters:
            {str(r)}
            has failed""", e)
        result = "na na"
    return result.stdout.replace(" ", ",")

def write_csv(filename: str, res: list[str]) -> None:
    with open(filename, "w", newline='') as f:
        for row in res:
            f.write(row)
            f.write("\n")
    

def main():
    try:
        subprocess.run(["make"], check=True)
    except subprocess.CalledProcessError as e:
        print("Build failed. Aborting benchmark.")
        sys.exit(1)

    threads = [4, 8, 16, 32, 64]
    contention = [0, 1, 2, 4, 8]
    op = [2, 8, 16, 64, 128]
    backoff = [0, 10, 100, 1000, 10000] # in microseconds

    csv_files = ['threads.csv', 'contention.csv', 'op.csv', 'backoff.csv']
    csv_files = [f"{res_dir}/{x}" for x in csv_files]

    threads_res = ["threads,combining,sequential"]
    contention_res = ["contention in millsec,combining,sequential"]
    op_res = ["ops per thread,combining,sequential"]
    backoff_res = ["backoff in microsec,combining,sequential"]

    runtime = Runtime()
    print("comparing number of threads...")
    for thread in threads:
        runtime.threads = thread
        print(str(runtime))
        res = benchmark(runtime)
        threads_res.append((f"{thread},{res}"))
    write_csv(csv_files[0], threads_res)
    print("done, results saved")

    runtime = Runtime()
    print("comparing thread contention...")
    for cont in contention:
        runtime.contention = cont
        print(str(runtime))
        res = benchmark(runtime)
        contention_res.append((f"{cont},{res}"))
    write_csv(csv_files[1], contention_res)
    print("done, results saved")

    runtime = Runtime()
    print("comparing operations per thread...")
    for o in op:
        runtime.op = o
        print(str(runtime))
        res = benchmark(runtime)
        op_res.append((f"{o},{res}"))
    write_csv(csv_files[2], op_res)
    print("done, results saved")

    runtime = Runtime()
    print("comparing spinlock backoff...")
    for back in backoff:
        runtime.backoff = back
        print(str(runtime))
        res = benchmark(runtime)
        backoff_res.append((f"{back},{res}"))
    write_csv(csv_files[3], backoff_res)
    print("done, results saved")

    print("\nbenchmarking complete")

main()