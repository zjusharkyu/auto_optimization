import os
import subprocess
import re
import statistics
import sys
import time

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
        # Linux 原生支持传入 CPU ID 的集合
        os.sched_setaffinity(pid, set(cpu_ids))
        return True
    except Exception as e:
        # 非 root 权限下可能会被拒绝，静默处理
        return False

def isolate_cpus(cpu_ids):
    """隔离指定 CPUs，尝试让其他进程不在此 CPUs 上运行"""
    try:
        target_cpus = set(cpu_ids)
        # 遍历 /proc 目录下的所有进程
        for pid_str in os.listdir('/proc'):
            if not pid_str.isdigit():
                continue
            pid = int(pid_str)
            
            # 跳过当前脚本及父进程
            if pid == os.getpid() or pid == os.getppid():
                continue
                
            try:
                current_affinity = os.sched_getaffinity(pid)
                # 将目标 CPU 从该进程的亲和性掩码中剔除
                new_affinity = current_affinity - target_cpus
                # 如果有交集且剔除后不为空，则设置新的掩码
                if new_affinity and new_affinity != current_affinity:
                    os.sched_setaffinity(pid, new_affinity)
            except (PermissionError, ProcessLookupError, OSError):
                # 忽略权限不足（非 root）或进程中途退出的情况
                pass
    except Exception as e:
        print(f"Warning: Could not isolate CPUs: {e}")

def set_process_realtime_priority(pid):
    """设置进程为实时优先级，防止被其他进程抢占 (Linux SCHED_FIFO)"""
    try:
        # 获取 SCHED_FIFO 的最高优先级 (通常是 99)
        max_priority = os.sched_get_priority_max(os.SCHED_FIFO)
        param = os.sched_param(max_priority)
        os.sched_setscheduler(pid, os.SCHED_FIFO, param)
        return True
    except Exception as e:
        # 非 root 环境无法设置实时优先级，静默处理
        return False

def main():
    print("=== Step 1: Build Project (Release Mode) ===")
    # 适配 Linux CMake：明确指定 CMAKE_BUILD_TYPE=Release
    success, out = run_cmd('cmake -B build -S . -DCMAKE_BUILD_TYPE=Release')
    if not success:
        print("[BUILD_FAILED] CMake Configuration Error:\n" + out)
        sys.exit(1)
        
    # 执行 Release 编译
    success, out = run_cmd('cmake --build build --config Release')
    if not success:
        print("[BUILD_FAILED] Compilation Error:\n" + out)
        sys.exit(1)
    print("Build successful.\n")

    print("=== Step 2: Run Correctness Tests (test.cpp) ===")
    # Linux 下的可执行文件通常没有 .exe 后缀
    test_exe = os.path.join('build', 'test')
    if not os.path.exists(test_exe):
        print("[TEST_FAILED] test binary not found. Build might have failed.")
        sys.exit(1)

    success, out = run_cmd(test_exe)
    if not success or "You got 14/14 tests correct." not in out:
        print("[TEST_FAILED] Correctness logic is broken! Tests failed:\n" + out)
        sys.exit(1)
    print("All tests passed (14/14).\n")

    print("=== Step 3: Run Benchmark (score.cpp) ===")
    score_exe = os.path.join('build', 'score')
    if not os.path.exists(score_exe):
        print("[SCORE_FAILED] score binary not found.")
        sys.exit(1)

    # 隔离 CPU 4 和 CPU 5，防止其他进程在上面运行
    isolate_cpus([4, 5])
    print("Isolated CPUs 4 and 5 for benchmark stability (Full isolation requires sudo)")

    iterations = 40  # 运行 40 次取中位数以减少系统抖动带来的干扰
    scores = []
    cpu_target_ids = [4, 5]  # Linux 下直接传入 ID 列表即可

    for i in range(iterations):
        # 启动进程但不等待完成
        process = subprocess.Popen(score_exe, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                  text=True, encoding='utf-8', errors='replace')

        # 设置进程 CPU 亲和性，绑定到 CPU 4 和 CPU 5
        if process.poll() is None:  # 进程仍在运行
            set_process_affinity(process.pid, cpu_target_ids)
            # 设置进程为实时优先级，防止被其他进程抢占
            set_process_realtime_priority(process.pid)

        # 等待进程完成并获取输出
        out, _ = process.communicate()
        retcode = process.returncode

        if retcode != 0:
            print(f"[SCORE_FAILED] Execution error on run {i+1}:\n" + out)
            sys.exit(1)

        # 解析 quantcup score.cpp 的输出
        lines =[line.strip() for line in out.split('\n') if line.strip()]
        if not lines:
            continue
        last_line = lines[-1]
        
        # 匹配正确的浮点数格式（带小数点的数字）
        matches = re.findall(r"\d+\.\d+", last_line)

        if matches:
            val = float(matches[-1])
            scores.append(val)
            print(f"  Run {i+1}/{iterations} | Score (Time): {val:.6f}")
        else:
            print(f"[SCORE_FAILED] Could not parse metric from: {last_line}")
            sys.exit(1)

    if not scores:
        print("[SCORE_FAILED] Failed to extract any scores.")
        sys.exit(1)

    median_score = statistics.median(scores)
    min_score = min(scores)
    
    print("\n=== Evaluation Complete ===")
    print(f"MIN_SCORE: {min_score:.2f}")
    print(f"MEDIAN_SCORE: {median_score:.2f}")
    print("NOTE: The score represents execution time. LOWER IS BETTER.")
    
    # 额外：保存结果供 Agent 静默读取决策
    with open("score_output.txt", "w", encoding="utf-8") as f:
        f.write(f"MEDIAN_SCORE: {median_score:.2f}\nMIN_SCORE: {min_score:.2f}\n")

if __name__ == "__main__":
    main()
