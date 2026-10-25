#!/usr/bin/env python3
"""Generate copy-ready LaTeX tables and local-search solution sections."""

from __future__ import annotations

import argparse
import re
from dataclasses import dataclass
from pathlib import Path


LOCAL_METHODS = [
    ("Greedy LS + node exchange + random", "Greedy + node exchange + random start",
     "greedy", "node_exchange", "random_start"),
    ("Greedy LS + node exchange + weighted NN", "Greedy + node exchange + weighted NN start",
     "greedy", "node_exchange", "weighted_nn_start"),
    ("Greedy LS + edge exchange + random", "Greedy + edge exchange + random start",
     "greedy", "edge_exchange", "random_start"),
    ("Greedy LS + edge exchange + weighted NN", "Greedy + edge exchange + weighted NN start",
     "greedy", "edge_exchange", "weighted_nn_start"),
    ("Steepest LS + node exchange + random", "Steepest + node exchange + random start",
     "steepest", "node_exchange", "random_start"),
    ("Steepest LS + node exchange + weighted NN", "Steepest + node exchange + weighted NN start",
     "steepest", "node_exchange", "weighted_nn_start"),
    ("Steepest LS + edge exchange + random", "Steepest + edge exchange + random start",
     "steepest", "edge_exchange", "random_start"),
    ("Steepest LS + edge exchange + weighted NN", "Steepest + edge exchange + weighted NN start",
     "steepest", "edge_exchange", "weighted_nn_start"),
]


@dataclass(frozen=True)
class Result:
    average: float
    minimum: float
    maximum: float
    time_average: float | None = None
    time_minimum: float | None = None
    time_maximum: float | None = None
    best_solution: str = ""

    def objective_text(self) -> str:
        return f"{self.average:.2f} ({self.minimum:.0f} -- {self.maximum:.0f})"

    def time_text(self) -> str:
        if self.time_average is None:
            return "---"
        if self.time_minimum is None or self.time_maximum is None:
            return f"{self.time_average:.3f} (---)"
        return (
            f"{self.time_average:.3f} "
            f"({self.time_minimum:.3f} -- {self.time_maximum:.3f})"
        )


def read_key_values(path: Path) -> dict[str, str]:
    values: dict[str, str] = {}
    for line in path.read_text().splitlines():
        if ": " in line:
            key, value = line.split(": ", 1)
            values[key] = value.strip()
    return values


def legacy_result(results_dir: Path, filename: str) -> Result:
    values = read_key_values(results_dir / filename)
    return Result(
        average=float(values["Average Cost"]),
        minimum=float(values["Best Cost"]),
        maximum=float(values["Worst Cost"]),
        time_average=float(values["Time"].removesuffix(" ms")) / 200.0,
    )


def local_result(results_dir: Path, dataset: str, method: tuple[str, ...]) -> Result:
    _, _, search, move, start = method
    filename = f"local_search_{dataset}_{search}_{move}_{start}.txt"
    values = read_key_values(results_dir / filename)
    return Result(
        average=float(values["Average Cost"]),
        minimum=float(values["Best Cost"]),
        maximum=float(values["Worst Cost"]),
        best_solution=values["Best Solution"],
    )


def local_report(results_dir: Path, dataset: str) -> dict[str, Result]:
    lines = (results_dir / f"local_search_report_{dataset}.txt").read_text().splitlines()
    results: dict[str, Result] = {}
    section = "objective"
    pattern = re.compile(
        r"^(.*?)\s+(-?\d+(?:\.\d+)?) \((-?\d+(?:\.\d+)?) - (-?\d+(?:\.\d+)?)\)$"
    )
    for line in lines:
        if line.startswith("Running times"):
            section = "time"
            continue
        match = pattern.match(line)
        if not match:
            continue
        name, average, minimum, maximum = match.groups()
        name = name.rstrip()
        result = results.get(name, Result(0, 0, 0))
        if section == "objective":
            results[name] = Result(float(average), float(minimum), float(maximum),
                                   result.time_average, result.time_minimum,
                                   result.time_maximum, result.best_solution)
        else:
            results[name] = Result(result.average, result.minimum, result.maximum,
                                   float(average), float(minimum), float(maximum),
                                   result.best_solution)
    return results


def all_results(results_dir: Path, dataset: str) -> list[tuple[str, str, Result, str]]:
    legacy = [
        ("Random solution", "assignment1", "random_solution_dataset_{i}.txt"),
        ("Nearest neighbor (end)", "assignment1", "nn_solution_dataset_{i}.txt"),
        ("Nearest neighbor (all positions)", "assignment1",
         "nn_anywhere_at_path_solution_dataset_{i}.txt"),
        ("Greedy cycle", "assignment1", "greedy_dataset_{i}.txt"),
        ("NN + 2-regret", "assignment2", "nn_anywhere_regret_dataset_{i}.txt"),
        ("NN + weighted sum", "assignment2", "nn_anywhere_weighted_dataset_{i}.txt"),
        ("GC + 2-regret", "assignment2", "greedy_cycle_regret_dataset_{i}.txt"),
        ("GC + weighted sum", "assignment2", "greedy_cycle_weighted_dataset_{i}.txt"),
    ]
    index = 0 if dataset == "TSPA" else 1
    rows = [
        (name, category, legacy_result(results_dir, filename.format(i=index)), "legacy")
        for name, category, filename in legacy
    ]
    report = local_report(results_dir, dataset)
    for short_name, report_name, *_ in LOCAL_METHODS:
        reported = report[report_name]
        stored = local_result(results_dir, dataset, next(
            method for method in LOCAL_METHODS if method[0] == short_name
        ))
        rows.append((
            short_name,
            "local_search",
            Result(
                reported.average,
                reported.minimum,
                reported.maximum,
                reported.time_average,
                reported.time_minimum,
                reported.time_maximum,
                stored.best_solution,
            ),
            "local",
        ))
    return rows


def decorate(value: str, bold: bool, underline: bool) -> str:
    if bold and underline:
        return rf"\textbf{{\underline{{{value}}}}}"
    if bold:
        return rf"\textbf{{{value}}}"
    if underline:
        return rf"\underline{{{value}}}"
    return value


def metric_text(value: float, all_values: list[float],
                category_values: list[float], decimals: int) -> str:
    format_string = f"{{:.{decimals}f}}"
    return decorate(
        format_string.format(value),
        value == min(all_values),
        value == min(category_values),
    )


def table_cell(rows: list[tuple[str, str, Result, str]], row: int,
               dataset: str) -> str:
    result = rows[row][2]
    category = rows[row][1]
    category_rows = [item for item in rows if item[1] == category]
    average = metric_text(
        result.average,
        [item[2].average for item in rows],
        [item[2].average for item in category_rows],
        0,
    )
    minimum = metric_text(
        result.minimum,
        [item[2].minimum for item in rows],
        [item[2].minimum for item in category_rows],
        0,
    )
    maximum = metric_text(
        result.maximum,
        [item[2].maximum for item in rows],
        [item[2].maximum for item in category_rows],
        0,
    )
    return f"{average} ({minimum} -- {maximum})"


def time_cell(rows: list[tuple[str, str, Result, str]], row: int) -> str:
    result = rows[row][2]
    if result.time_average is None:
        return "---"
    category = rows[row][1]
    category_rows = [item for item in rows if item[1] == category]
    average = metric_text(
        result.time_average,
        [item[2].time_average for item in rows
         if item[2].time_average is not None],
        [item[2].time_average for item in category_rows
         if item[2].time_average is not None],
        3,
    )
    if result.time_minimum is None or result.time_maximum is None:
        return f"{average} (---)"

    minimum = metric_text(
        result.time_minimum,
        [item[2].time_minimum for item in rows
         if item[2].time_minimum is not None],
        [item[2].time_minimum for item in category_rows
         if item[2].time_minimum is not None],
        3,
    )
    maximum = metric_text(
        result.time_maximum,
        [item[2].time_maximum for item in rows
         if item[2].time_maximum is not None],
        [item[2].time_maximum for item in category_rows
         if item[2].time_maximum is not None],
        3,
    )
    return f"{average} ({minimum} -- {maximum})"


def objective_table(tspa: list[tuple[str, str, Result, str]],
                    tspb: list[tuple[str, str, Result, str]]) -> str:
    lines = [
        r"\begin{table}[h!]", r"\centering", r"\footnotesize",
        r"\setlength{\tabcolsep}{3pt}", r"\renewcommand{\arraystretch}{1.5}",
        r"\caption{Objective function values reported as \texttt{average (min -- max)} across 200 runs for Instance A and Instance B. Each average, minimum, and maximum is highlighted independently: the best overall value is bold and the best value within a method category is underlined. NN = nearest neighbor (all positions), GC = greedy cycle, LS = local search.}",
        r"\label{tab:obj_results}", r"\vspace{0.5em}",
        r"\begin{tabularx}{\textwidth}{p{7.2cm} X X}", r"\toprule",
        r"\textbf{Method} & \textbf{Instance A: av (min -- max)} & \textbf{Instance B: av (min -- max)} \\",
        r"\midrule",
    ]
    for index, (name, category, *_rest) in enumerate(tspa):
        if index in (4, 8):
            lines.append(r"\cmidrule{1-3}")
        lines.append(
            f"{name} & {table_cell(tspa, index, 'TSPA')} & "
            f"{table_cell(tspb, index, 'TSPB')} \\\\"
        )
    lines += [r"\bottomrule", r"\end{tabularx}", r"\end{table}"]
    return "\n".join(lines)


def time_table(tspa: list[tuple[str, str, Result, str]],
               tspb: list[tuple[str, str, Result, str]]) -> str:
    lines = [
        r"\begin{table}[h!]", r"\centering", r"\footnotesize",
        r"\setlength{\tabcolsep}{3pt}", r"\renewcommand{\arraystretch}{1.5}",
        r"\caption{Running times in milliseconds reported as \texttt{average (min -- max)} per single run across 200 runs for Instance A and Instance B. Each available average, minimum, and maximum is highlighted independently: the best overall value is bold and the best value within a method category is underlined.}",
        r"\label{tab:time_results}", r"\vspace{0.5em}",
        r"\begin{tabularx}{\textwidth}{p{7.2cm} X X}", r"\toprule",
        r"\textbf{Method} & \textbf{Instance A [ms]: av (min -- max)} & \textbf{Instance B [ms]: av (min -- max)} \\",
        r"\midrule",
    ]
    for index, (name, *_rest) in enumerate(tspa):
        if index in (4, 8):
            lines.append(r"\cmidrule{1-3}")
        lines.append(
            f"{name} & {time_cell(tspa, index)} & {time_cell(tspb, index)} \\\\"
        )
    lines += [r"\bottomrule", r"\end{tabularx}", r"\end{table}"]
    return "\n".join(lines)


def solution_sections(results_dir: Path, dataset: str) -> str:
    rows = all_results(results_dir, dataset)
    local_rows = [row for row in rows if row[3] == "local"]
    lines = [rf"\subsection{{Instance {'A' if dataset == 'TSPA' else 'B'}}}",
             r"\begin{itemize}[leftmargin=1.5em]"]
    for _, report_name, search, move, start in LOCAL_METHODS:
        result = next(row[2] for row in local_rows if row[0] == _)
        title = report_name
        lines += [
            rf"    \item \textbf{{{title}}} (Obj: \texttt{{{result.minimum:.0f}}}):\\",
            rf"    \texttt{{{result.best_solution}}}",
        ]
    lines += [r"\end{itemize}"]
    return "\n".join(lines)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--results-dir", type=Path, default=Path("calculated_solutions"))
    parser.add_argument("--output", type=Path,
                        default=Path("calculated_solutions/local_search_latex.tex"))
    args = parser.parse_args()

    tspa = all_results(args.results_dir, "TSPA")
    tspb = all_results(args.results_dir, "TSPB")
    output = "\n\n".join([
        objective_table(tspa, tspb),
        time_table(tspa, tspb),
        solution_sections(args.results_dir, "TSPA"),
        solution_sections(args.results_dir, "TSPB"),
    ]) + "\n"
    args.output.write_text(output)
    print(output, end="")
    print(f"\nWrote LaTeX to {args.output}", flush=True)


if __name__ == "__main__":
    main()
