import os
import subprocess
import statistics
import sys
import re

def run_cmd(cmd):
    """执行终端命令并返回是否成功及输出"""
    try:
        result = subprocess.run(cmd, shell=True, check=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                              text=True, encoding='utf-8', errors='replace')
        return True, result.stdout
    except subprocess.CalledProcessError as e:
        return False, e.stdout

def set_process_affinity(pid, cpu_ids):
    """将进程绑定到指定 CPU 核心 (Linux Native)"""
    try:
        os.sched_setaffinity(pid, set(cpu_ids))
        return True
    except Exception as e:
        return False

def set_process_realtime_priority(pid):
    """设置进程为实时优先级，防止被其他进程抢占 (Linux SCHED_FIFO)"""
    try:
        max_priority = os.sched_get_priority_max(os.SCHED_FIFO)
        param = os.sched_param(max_priority)
        os.sched_setscheduler(pid, os.SCHED_FIFO, param)
        return True
    except Exception as e:
        return False

def main():
    print("=== Step 1: Run Correctness Tests (test.cpp) ===")
    test_exe = './test'
    if not os.path.exists(test_exe):
        print(f"[TEST_FAILED] {test_exe} not found.")
        sys.exit(1)

    success, out = run_cmd(test_exe)
    if not success or "You got 14/14 tests correct." not in out:
        print("[TEST_FAILED] Tests failed:\n" + out)
        sys.exit(1)
    print("All tests passed (14/14).\n")

    print("=== Step 2: Run Benchmark (score.cpp) ===")
    score_exe = './score'
    if not os.path.exists(score_exe):
        print(f"[SCORE_FAILED] {score_exe} not found.")
        sys.exit(1)

    iterations = 20  # 运行 20 次取中位数
    scores = []
    cpu_target_ids = [0, 1]  # 使用 CPU 0 和 1

    for i in range(iterations):
        process = subprocess.Popen(score_exe, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                  text=True, encoding='utf-8', errors='replace')

        if process.poll() is None:
            set_process_affinity(process.pid, cpu_target_ids)
            set_process_realtime_priority(process.pid)

        out, _ = process.communicate()
        retcode = process.returncode

        if retcode != 0:
            print(f"[SCORE_FAILED] Execution error on run {i+1}:\n" + out)
            sys.exit(1)

        lines = [line.strip() for line in out.split('\n') if line.strip()]
        if not lines:
            continue
        last_line = lines[-1]

        matches = re.findall(r"\d+\.\d+", last_line)

        if matches:
            val = float(matches[-1])
            scores.append(val)
            print(f"  Run {i+1}/{iterations} | Score (Time): {val:.6f}")
        else:
            print(f"[SCORE_FAILED] Could not parse metric from: {last_line}")

    if not scores:
        print("[SCORE_FAILED] Failed to extract any scores.")
        sys.exit(1)

    median_score = statistics.median(scores)
    min_score = min(scores)
    mean_score = statistics.mean(scores)

    print("\n=== Evaluation Complete ===")
    print(f"MIN_SCORE:   {min_score:.2f}")
    print(f"MEDIAN_SCORE: {median_score:.2f}")
    print(f"MEAN_SCORE:  {mean_score:.2f}")
    print("NOTE: The score represents execution time. LOWER IS BETTER.")

    with open("score_output.txt", "w", encoding="utf-8") as f:
        f.write(f"MEDIAN_SCORE: {median_score:.2f}\nMIN_SCORE: {min_score:.2f}\nMEAN_SCORE: {mean_score:.2f}\n")

if __name__ == "__main__":
    main()
