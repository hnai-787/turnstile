import os
import subprocess
import datetime
from pathlib import Path
from difflib import unified_diff
import matplotlib.pyplot as plt
import base64
from io import BytesIO
import re

SHELL_SCRIPT = "./kernel_baseline_test.sh"

def run_shell_script(tag):
    result_dir = Path(f"kernel_{tag}")
    print(f"[+] Running shell script test, saving to {result_dir}")
    env = os.environ.copy()
    env['RESULTS'] = str(result_dir.resolve())
    result_dir.mkdir(parents=True, exist_ok=True)
    subprocess.run([SHELL_SCRIPT], env=env, check=True)
    print(f"[✓] Shell script test complete for {tag}")
    return result_dir

def extract_metrics(file_path):
    metrics = {}
    with open(file_path) as f:
        for line in f:
            match = re.match(r'^[\s]*([\w\s\./-]+):[\s]*([0-9.]+)', line)
            if match:
                key = match.group(1).strip()
                val = match.group(2).strip()
                metrics[key] = val
    return metrics

def plot_comparison_chart(title, metrics_old, metrics_new):
    keys = list(metrics_old.keys() & metrics_new.keys())
    values_old = [float(metrics_old[k]) for k in keys]
    values_new = [float(metrics_new[k]) for k in keys]

    fig, ax = plt.subplots(figsize=(10, 5))
    x = range(len(keys))
    ax.bar(x, values_old, width=0.4, label='Baseline', align='center')
    ax.bar([i + 0.4 for i in x], values_new, width=0.4, label='Modified', align='center')
    ax.set_xticks([i + 0.2 for i in x])
    ax.set_xticklabels(keys, rotation=45, ha='right')
    ax.set_title(title)
    ax.legend()

    buffer = BytesIO()
    plt.tight_layout()
    plt.savefig(buffer, format='png')
    buffer.seek(0)
    encoded = base64.b64encode(buffer.read()).decode('utf-8')
    plt.close(fig)
    return f'<img src="data:image/png;base64,{encoded}" />'

def compare_results(old_dir: Path, new_dir: Path, report_name="kernel_comparison_report.html"):
    print("[+] Comparing results...")
    report = [
        "<html><head><title>Kernel Test Comparison</title><style>",
        "body { font-family: Arial, sans-serif; background: #ffffff; color: #000; padding: 20px; }",
        "h1, h2, h3 { color: #1a1a1a; }",
        "table { width: 100%; border-collapse: collapse; margin-bottom: 20px; }",
        "td, th { border: 1px solid #ccc; padding: 8px; vertical-align: top; font-size: 14px; }",
        "th { background-color: #f2f2f2; }",
        "code { white-space: pre-wrap; display: block; background: #f8f8f8; padding: 10px; border: 1px solid #ccc; }",
        "img { max-width: 100%; height: auto; }",
        "</style></head><body><h1>Kernel Comparison Report</h1>"
    ]

    report.append(f"<p><strong>Baseline:</strong> {old_dir.name} &nbsp; | &nbsp; <strong>Modified:</strong> {new_dir.name}</p>")

    score = {"baseline": 0, "modified": 0}

    for f in ["sysbench_cpu.txt", "stressng_cpu.txt", "perf_sched_latency.txt", "sysbench_fileio.txt", "top_snapshot.txt"]:
        old_path = old_dir / f
        new_path = new_dir / f
        if old_path.exists() and new_path.exists():
            report.append(f"<h2>{f.replace('.txt','').replace('_',' ').title()}</h2>")
            winner = "unknown"
            try:
                old_metrics = extract_metrics(old_path)
                new_metrics = extract_metrics(new_path)
                if old_metrics and new_metrics:
                    report.append(plot_comparison_chart(f, old_metrics, new_metrics))

                    # Simple scoring logic: sum values and compare
                    old_total = sum(float(v) for v in old_metrics.values())
                    new_total = sum(float(v) for v in new_metrics.values())
                    if new_total < old_total:
                        winner = "Modified Kernel"
                        score["modified"] += 1
                    elif old_total < new_total:
                        winner = "Baseline Kernel"
                        score["baseline"] += 1
                    else:
                        winner = "Draw"
            except Exception as e:
                report.append(f"<p><em>Unable to parse metrics for chart: {e}</em></p>")

            report.append(f"<p><strong>Round Winner:</strong> {winner}</p>")

            with open(old_path) as f1, open(new_path) as f2:
                old_lines = f1.readlines()
                new_lines = f2.readlines()
                diff = list(unified_diff(old_lines, new_lines, fromfile=old_path.name, tofile=new_path.name))
                if diff:
                    report.append("<h3>Detailed Output Differences</h3><code>" + ''.join(diff) + "</code>")
                else:
                    report.append("<p>No significant line-by-line differences detected.</p>")

    # Overall conclusion
    report.append("<hr><h2>Overall Conclusion</h2>")
    if score["modified"] > score["baseline"]:
        report.append(f"<p><strong>Winner:</strong> Modified Kernel ({score['modified']} vs {score['baseline']})</p>")
    elif score["baseline"] > score["modified"]:
        report.append(f"<p><strong>Winner:</strong> Baseline Kernel ({score['baseline']} vs {score['modified']})</p>")
    else:
        report.append("<p><strong>Result:</strong> Draw</p>")

    report.append("</body></html>")

    with open(report_name, "w") as f:
        f.writelines(report)
    print(f"[✓] HTML comparison report with charts saved to {report_name}")

if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser(description="Kernel performance test and comparison")
    parser.add_argument("--baseline", help="Run and label this set as baseline")
    parser.add_argument("--new", help="Run and label this set as new")
    parser.add_argument("--compare", nargs=2, metavar=('OLD', 'NEW'), help="Compare two result sets")
    args = parser.parse_args()

    if args.baseline:
        run_shell_script(args.baseline)
    elif args.new:
        run_shell_script(args.new)
    elif args.compare:
        compare_results(
            Path(f"kernel_{args.compare[0]}"),
            Path(f"kernel_{args.compare[1]}")
        )
    else:
        parser.print_help()
