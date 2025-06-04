import subprocess
import csv
import sys
import re
import pandas as pd
import plots

this_dir = "/home/jjurgenson/code_stuff/CombiningCounterTest/test"
command = f"{this_dir}/combining_tree_test" # (cores, work, runs, op, backoff)
res_dir = f"{this_dir}/results"
media_dir = f"{this_dir}/media/run4"

class Runtime:
    def __init__(self, params: dict = None):
        defaults = {
            "cores": '12',
            "work": '0',
            "runs": '10',
            "op": '4096',
            "backoff": '10',
            "threads_per_core": '1'
        }
        if params:
            defaults.update(params)
        self.__dict__.update(defaults)
    
    def __str__(self):
        return f"cores: {self.cores}, work: {self.work}, op: {self.op}, threads per core: {self.threads_per_core}"
    
    def __call__(self) -> list:
        result = None
        try:
            result = subprocess.run(
                [command, self.cores, self.work, self.runs, self.op, self.backoff, self.threads_per_core],
                capture_output=True,
                text=True,
                check=True
            )
        except Exception as e:
            print(f"""run with parameters:
                {str(self)}
                has failed""", e)
            result = "na na"
        return result.stdout.split()

def write_csv(filename: str, res) -> None:
    with open(filename, "w", newline='') as f:
        writer = csv.writer(f)
        writer.writerows(res)
    print("csv file written")
    

def main():
    try:
        subprocess.run(["make"], check=True)
    except subprocess.CalledProcessError as e:
        print("Build failed. Aborting benchmark.")
        sys.exit(1)

    # prepare system for benchmark
    #subprocess.run(['python', 'benchmark_helper.py', 'prepare', '--cores', '0-11'])

    cores = ['4', '8', '9', '10', '11', '12']
    threads = ['16', '32']
    work = ['0', '100', '200', '1000']

    processors_header = ["processors", "combining", "sequential"]
    processors_res = []
    processors_files = []
    work_header = ["work", "combining", "sequential"]
    work_res = []
    work_files = []

    print("beginning benchmarking...")

    for thread in threads:
        processors_res.append(processors_header)
        for core in cores:
            params = {"cores": core, "threads_per_core": thread}
            runtime = Runtime(params)
            print(runtime)
            res = runtime()
            processors_res.append([f'{core}'] + res)
        file = f"{res_dir}/{'processors'}_t_{thread}.csv"
        processors_files.append(file)
        write_csv(file, processors_res)
        processors_res = []
    
    for core in cores:
        work_res.append(work_header)
        for w in work:
            params = {"cores": core, "work": w}
            runtime = Runtime(params)
            print(runtime)
            # res = runtime()
            # work_res.append([f'{w}'] + res)
        file = f"{res_dir}/{'work'}_c_{core}.csv"
        work_files.append(file)
        # write_csv(file, work_res)
        work_res = []

    print("benchmarking complete")
   
    # restore system
#    subprocess.run(['python', 'benchmark_helper.py', 'restore'])

    default = Runtime()
    op_per_thread = default.op
    just_name = lambda s: re.split('[/.]', s)[-2]
    just_const = lambda s: re.split('[_.]', s)[-2]
    for file in processors_files:
        df = pd.read_csv(file)
        title = f"{op_per_thread} increments per thread, {just_const(file)} thread(s) per processor"
        filename = f"{media_dir}/{just_name(file)}.png"
        plots.plot_both(df, "Processors", "processors", title, filename)
    # for file in work_files:
    #     df = pd.read_csv(file)
    #     title = f"{op_per_thread} increments per thread, {just_const(file)} processors ({default.threads_per_core} thread(s) per processor)"
    #     filename = f"{media_dir}/{just_name(file)}.png"
    #     plots.plot_both(df, "Work", "work", title, filename)
    
    print("plots complete")

if __name__ == '__main__':
	main()
