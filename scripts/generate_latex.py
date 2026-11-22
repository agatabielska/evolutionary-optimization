"""Generate copy-ready LaTeX tables and solution sections for tasks 3--5.

Reads the files written by main.cpp from calculated_solutions/task<N>/ and writes
the LaTeX output into the corresponding task directory.
"""

from __future__ import annotations

import argparse
import re
from dataclasses import dataclass, replace
from pathlib import Path
from typing import NamedTuple

DATASETS = ("TSPA", "TSPB")
INSTANCE = {"TSPA": "A", "TSPB": "B"}

OBJECTIVE_FIELDS = ("average", "minimum", "maximum")
TIME_FIELDS = ("time_average", "time_minimum", "time_maximum")

@dataclass(frozen=True)
class Result:
    average: float = 0.0
    minimum: float = 0.0
    maximum: float = 0.0
    time_average: float | None = None
    time_minimum: float | None = None
    time_maximum: float | None = None
    evaluated: float | None = None
    generated: float | None = None   # task 5: average moves added to LM per run
    applied: float | None = None
    lists_ms: float | None = None
    best_solution: str = ""


class Row(NamedTuple):
    name: str       # name shown in the table
    category: str   # rows of the same category are compared for the underline
    result: Result

def solution_file(results_dir: Path, task: int, dataset: str, description: str) -> Path:
    return results_dir / f"task{task}" / f"task{task}-solution-{dataset.lower()}-{description}.txt"


def report_file(results_dir: Path, task: int, dataset: str, description: str = "") -> Path:
    suffix = f"-{description}" if description else ""
    return results_dir / f"task{task}" / f"task{task}-report-{dataset.lower()}{suffix}.txt"


def read_key_values(path: Path) -> dict[str, str]:
    values: dict[str, str] = {}
    for line in path.read_text().splitlines():
        if ": " in line:
            key, value = line.split(": ", 1)
            values[key] = value.strip()
    return values


def parse_number(text: str) -> float | None:
    return None if text == "-" else float(text)


REPORT_LINE = re.compile(
    r"^(.*?)(-?\d+(?:\.\d+)?)\s+\((-?\d+(?:\.\d+)?)\s+-\s+(-?\d+(?:\.\d+)?)\)$"
)


def read_report(path: Path) -> dict[str, Result]:
    """Objective and running-time tables written by write_local_search_report."""
    results: dict[str, Result] = {}
    in_time_section = False
    for line in path.read_text().splitlines():
        if line.startswith("Running times"):
            in_time_section = True
            continue
        match = REPORT_LINE.match(line)
        if not match:
            continue
        name = match.group(1).rstrip()
        average, minimum, maximum = (float(value) for value in match.groups()[1:])
        current = results.get(name, Result())
        if in_time_section:
            results[name] = replace(current, time_average=average,
                                    time_minimum=minimum, time_maximum=maximum)
        else:
            results[name] = replace(current, average=average,
                                    minimum=minimum, maximum=maximum)
    return results


MOVES_LINE = re.compile(r"^(.*?)\s+(-|[\d.]+)\s+(-|[\d.]+)\s+(-|[\d.]+)$")


def read_moves(path: Path) -> dict[str, Result]:
    """Evaluated / applied moves table written by write_task4_stats."""
    results: dict[str, Result] = {}
    for line in path.read_text().splitlines():
        match = MOVES_LINE.match(line)
        if not match or match.group(1).strip() == "Method":
            continue
        evaluated, applied, lists_ms = (parse_number(value) for value in match.groups()[1:])
        results[match.group(1).rstrip()] = Result(
            evaluated=evaluated, applied=applied, lists_ms=lists_ms)
    return results


TASK5_MOVES_LINE = re.compile(
    r"(-|\d+(?:\.\d+)?)\s+(-|\d+(?:\.\d+)?)\s+"
    r"(-|\d+(?:\.\d+)?)\s+(-|\d+(?:\.\d+)?)\s*$"
)


def read_task5_moves(path: Path) -> dict[str, Result]:
    """Evaluated, added-to-LM, and applied move counts written by Task 5."""
    results: dict[str, Result] = {}
    for line in path.read_text().splitlines():
        match = TASK5_MOVES_LINE.search(line)
        if not match:
            continue
        method_name = line[:match.start()].rstrip()
        if method_name == "Method":
            continue
        evaluated, generated, applied, lists_ms = (
            parse_number(value) for value in match.groups()
        )
        results[method_name] = Result(
            evaluated=evaluated, generated=generated, applied=applied,
            lists_ms=lists_ms
        )
    return results


TASK7_ITERATIONS_LINE = re.compile(
    r"^(.*?)\s+(-|\d+(?:\.\d+)?)\s+\((\d+)\s+-\s+(\d+)\)$"
)


def read_task7_iterations(path: Path) -> dict[str, tuple[float, int, int]]:
    results: dict[str, tuple[float, int, int]] = {}
    for line in path.read_text().splitlines():
        match = TASK7_ITERATIONS_LINE.match(line)
        if not match or match.group(1).strip() == "Method":
            continue
        average = None if match.group(2) == "-" else float(match.group(2))
        if average is not None:
            results[match.group(1).rstrip()] = (
                average, int(match.group(3)), int(match.group(4)))
    return results


def read_combined_solutions(path: Path) -> dict[str, tuple[int, str]]:
    results: dict[str, tuple[int, str]] = {}
    lines = path.read_text().splitlines()
    for index in range(0, len(lines), 2):
        header = lines[index]
        name = header[:header.find(" | best cost")]
        cost = int(header.split(" | best cost ", 1)[1])
        results[name] = (cost, lines[index + 1])
    return results


def best_solution(results_dir: Path, task: int, dataset: str, description: str) -> str:
    return read_key_values(solution_file(results_dir, task, dataset, description))["Best Solution"]

# (table name, category, task, description of the solution file)
EARLIER_METHODS = [
    ("Random solution", "assignment1", 1, "random"),
    ("Nearest neighbor (end)", "assignment1", 1, "nn-end"),
    ("Nearest neighbor (all positions)", "assignment1", 1, "nn-any"),
    ("Greedy cycle", "assignment1", 1, "greedy-cycle"),
    ("NN + 2-regret", "assignment2", 2, "nn-regret"),
    ("NN + weighted sum", "assignment2", 2, "nn-weighted"),
    ("GC + 2-regret", "assignment2", 2, "cycle-regret"),
    ("GC + weighted sum", "assignment2", 2, "cycle-weighted"),
]

# (table name, name in the task 3 report, description of the solution file)
LOCAL_METHODS = [
    ("Greedy LS + node exchange + random", "Greedy + node exchange + random start",
     "greedy-node-random-start"),
    ("Greedy LS + node exchange + weighted NN", "Greedy + node exchange + weighted NN start",
     "greedy-node-greedy-start"),
    ("Greedy LS + edge exchange + random", "Greedy + edge exchange + random start",
     "greedy-edge-random-start"),
    ("Greedy LS + edge exchange + weighted NN", "Greedy + edge exchange + weighted NN start",
     "greedy-edge-greedy-start"),
    ("Steepest LS + node exchange + random", "Steepest + node exchange + random start",
     "steepest-node-random-start"),
    ("Steepest LS + node exchange + weighted NN", "Steepest + node exchange + weighted NN start",
     "steepest-node-greedy-start"),
    ("Steepest LS + edge exchange + random", "Steepest + edge exchange + random start",
     "steepest-edge-random-start"),
    ("Steepest LS + edge exchange + weighted NN", "Steepest + edge exchange + weighted NN start",
     "steepest-edge-greedy-start"),
]

CANDIDATE_COUNTS = (5, 10, 15, 20)

# (table name, category, name in the task 4 report, description of the solution file)
TASK4_METHODS = [
    ("Steepest LS (baseline)", "baseline",
     "Steepest + edge exchange + random start (baseline)", "steepest-baseline"),
] + [
    (f"Steepest LS + candidate moves ($k = {k}$)", "candidates",
     f"Steepest + candidate moves (k = {k}) + random start", f"steepest-candidates-k{k}")
    for k in CANDIDATE_COUNTS
]

TASK5_METHODS = [
    ("Steepest LS (baseline)", "Steepest + edge exchange + random start (baseline)",
     "steepest-baseline"),
    ("Steepest LS + improving move list", "Steepest + improving move list + random start",
     "improving-list"),
] + [
    (f"Steepest LS + improving move list + candidates ($k = {k}$)",
     f"Steepest + improving move list + candidate moves (k = {k}) + random start",
     f"improving-list-candidates-k{k}")
    for k in CANDIDATE_COUNTS
]


def earlier_rows(results_dir: Path, dataset: str) -> list[Row]:
    rows = []
    for name, category, task, description in EARLIER_METHODS:
        values = read_key_values(solution_file(results_dir, task, dataset, description))
        rows.append(Row(name, category, Result(
            average=float(values["Average Cost"]),
            minimum=float(values["Best Cost"]),
            maximum=float(values["Worst Cost"]),
            # these files store the total time of all 200 runs
            time_average=float(values["Time"].removesuffix(" ms")) / 200.0,
        )))
    return rows


def task3_rows(results_dir: Path, dataset: str) -> list[Row]:
    report = read_report(report_file(results_dir, 3, dataset))
    rows = earlier_rows(results_dir, dataset)
    for name, report_name, description in LOCAL_METHODS:
        rows.append(Row(name, "local_search", replace(
            report[report_name],
            best_solution=best_solution(results_dir, 3, dataset, description))))
    return rows


def task4_rows(results_dir: Path, dataset: str) -> list[Row]:
    report = read_report(report_file(results_dir, 4, dataset))
    moves = read_moves(report_file(results_dir, 4, dataset, "moves"))
    rows = []
    for name, category, report_name, description in TASK4_METHODS:
        counted = moves[report_name]
        rows.append(Row(name, category, replace(
            report[report_name],
            evaluated=counted.evaluated, applied=counted.applied, lists_ms=counted.lists_ms,
            best_solution=best_solution(results_dir, 4, dataset, description))))
    return rows


def task5_rows(results_dir: Path, dataset: str) -> list[Row]:
    report = read_report(report_file(results_dir, 5, dataset, "combined"))
    moves = read_task5_moves(report_file(results_dir, 5, dataset, "combined-moves"))
    rows = []
    for name, report_name, description in TASK5_METHODS:
        counted = moves[report_name]
        rows.append(Row(name, "baseline" if description == "steepest-baseline"
                        else "improving_list", replace(
                            report[report_name],
                            evaluated=counted.evaluated,
                            generated=counted.generated,
                            applied=counted.applied,
                            best_solution=best_solution(
                                results_dir, 5, dataset, f"combined-{description}"))))
    return rows


TASK7_METHOD_NAMES = [
    "LNS without LS - random 30 nodes",
    "LNS without LS - single random subpath (30 nodes)",
    "LNS without LS - 3-5 random subpaths (30 nodes)",
    "LNS with LS - random 30 nodes",
    "LNS with LS - single random subpath (30 nodes)",
    "LNS with LS - 3-5 random subpaths (30 nodes)",
]

TASK7_HEURISTIC_NAMES = [
    "Optional heuristic LNS without LS",
    "Optional heuristic LNS with LS",
]


def task7_rows(results_dir: Path, dataset: str) -> list[Row]:
    report = read_report(report_file(results_dir, 7, dataset))
    solutions = read_combined_solutions(
        results_dir / "task7" / f"task7-solution-{dataset.lower()}.txt")
    rows = []
    for name in TASK7_METHOD_NAMES:
        cost, best = solutions[name]
        category = ("lns_with_ls" if name.startswith("LNS with LS")
                    else "lns_without_ls" if name.startswith("LNS without LS")
                    else "lns")
        rows.append(Row(name, category, replace(
            report[name], minimum=cost, best_solution=best)))

    heuristic_report = read_report(
        report_file(results_dir, 7, dataset, "heuristic"))
    heuristic_solutions = read_combined_solutions(
        report_file(results_dir, 7, dataset, "heuristic")
        .with_name(f"task7-solution-{dataset.lower()}-heuristic.txt"))
    for name in TASK7_HEURISTIC_NAMES:
        cost, best = heuristic_solutions[name]
        rows.append(Row(name, "lns_heuristic", replace(
            heuristic_report[name], minimum=cost, best_solution=best)))
    return rows


def decorate(value: str, bold: bool, underline: bool) -> str:
    if bold and underline:
        return rf"\textbf{{\underline{{{value}}}}}"
    if bold:
        return rf"\textbf{{{value}}}"
    if underline:
        return rf"\underline{{{value}}}"
    return value


def highlighted(rows: list[Row], index: int, field: str, decimals: int,
                use_categories: bool) -> str:
    """Value of one field; the best overall is bold, the best in its category underlined."""
    value = getattr(rows[index].result, field)
    category = rows[index].category
    overall = [v for row in rows if (v := getattr(row.result, field)) is not None]
    in_category = [v for row in rows
                   if row.category == category and (v := getattr(row.result, field)) is not None]
    return decorate(f"{value:.{decimals}f}", value == min(overall),
                    use_categories and value == min(in_category))


def triple_cell(rows: list[Row], index: int, fields: tuple[str, str, str],
                decimals: int, use_categories: bool) -> str:
    """'average (min -- max)' with independent highlighting of the three numbers."""
    result = rows[index].result
    average_field, minimum_field, maximum_field = fields
    if getattr(result, average_field) is None:
        return "---"
    average = highlighted(rows, index, average_field, decimals, use_categories)
    if getattr(result, minimum_field) is None or getattr(result, maximum_field) is None:
        return f"{average} (---)"
    minimum = highlighted(rows, index, minimum_field, decimals, use_categories)
    maximum = highlighted(rows, index, maximum_field, decimals, use_categories)
    return f"{average} ({minimum} -- {maximum})"


def table_lines(caption: str, label: str, column_spec: str, header: str,
                body: list[str]) -> str:
    lines = [
        r"\begin{table}[h!]", r"\centering", r"\footnotesize",
        r"\setlength{\tabcolsep}{3pt}", r"\renewcommand{\arraystretch}{1.5}",
        rf"\caption{{{caption}}}", rf"\label{{{label}}}", r"\vspace{0.5em}",
        rf"\begin{{tabularx}}{{\textwidth}}{{{column_spec}}}", r"\toprule",
        header, r"\midrule",
        *body,
        r"\bottomrule", r"\end{tabularx}", r"\end{table}",
    ]
    return "\n".join(lines)


def metric_table(rows_a: list[Row], rows_b: list[Row], *, fields: tuple[str, str, str],
                 decimals: int, caption: str, label: str, header: str,
                 separators: tuple[int, ...], use_categories: bool) -> str:
    body = []
    for index, row in enumerate(rows_a):
        if index in separators:
            body.append(r"\cmidrule{1-3}")
        body.append(
            f"{row.name} & {triple_cell(rows_a, index, fields, decimals, use_categories)} & "
            f"{triple_cell(rows_b, index, fields, decimals, use_categories)} \\\\"
        )
    return table_lines(caption, label, r"p{7.2cm} X X", header, body)


def moves_cell(rows: list[Row], index: int, field: str, best_is_bold: bool) -> str:
    value = getattr(rows[index].result, field)
    if value is None:
        return "---"
    if not best_is_bold:
        return f"{value:.0f}"
    return highlighted(rows, index, field, 0, use_categories=False)


def moves_table(rows_a: list[Row], rows_b: list[Row], caption: str, label: str) -> str:
    header = (
        r"\textbf{Method} & \textbf{A: evaluated} & \textbf{A: applied} & "
        r"\textbf{B: evaluated} & \textbf{B: applied} \\"
    )
    body = []
    for index, row in enumerate(rows_a):
        if index == 1:
            body.append(r"\cmidrule{1-5}")
        body.append(
            f"{row.name} & {moves_cell(rows_a, index, 'evaluated', True)} & "
            f"{moves_cell(rows_a, index, 'applied', False)} & "
            f"{moves_cell(rows_b, index, 'evaluated', True)} & "
            f"{moves_cell(rows_b, index, 'applied', False)} \\\\"
        )
    return table_lines(caption, label, r"p{7.2cm} X X X X", header, body)


def task5_moves_table(rows_a: list[Row], rows_b: list[Row],
                      caption: str, label: str) -> str:
    header = (
        r"\textbf{Method} & \textbf{A: evaluated} & \textbf{A: added} & "
        r"\textbf{A: applied} & \textbf{B: evaluated} & \textbf{B: added} & "
        r"\textbf{B: applied} \\"
    )
    body = []
    for index, row in enumerate(rows_a):
        if index == 1:
            body.append(r"\cmidrule{1-7}")
        body.append(
            f"{row.name} & {moves_cell(rows_a, index, 'evaluated', True)} & "
            f"{moves_cell(rows_a, index, 'generated', False)} & "
            f"{moves_cell(rows_a, index, 'applied', False)} & "
            f"{moves_cell(rows_b, index, 'evaluated', True)} & "
            f"{moves_cell(rows_b, index, 'generated', False)} & "
            f"{moves_cell(rows_b, index, 'applied', False)} \\\\"
        )
    return table_lines(caption, label, r"p{5.8cm} X X X X X X", header, body)


def solution_section(dataset: str, rows: list[Row]) -> str:
    lines = [rf"\subsection{{Instance {INSTANCE[dataset]}}}",
             r"\begin{itemize}[leftmargin=1.5em]"]
    for row in rows:
        lines += [
            rf"    \item \textbf{{{row.name}}} (Obj: \texttt{{{row.result.minimum:.0f}}}):\\",
            rf"    \texttt{{{row.result.best_solution}}}",
        ]
    lines.append(r"\end{itemize}")
    return "\n".join(lines)


# --------------------------------------------------------------------------------------
# Whole documents
# --------------------------------------------------------------------------------------

def task3_latex(results_dir: Path) -> str:
    tspa, tspb = (task3_rows(results_dir, dataset) for dataset in DATASETS)
    separators = (4, 8)  # after assignment 1 and after assignment 2
    objective = metric_table(
        tspa, tspb, fields=OBJECTIVE_FIELDS, decimals=0, separators=separators,
        use_categories=True, label="tab:obj_results",
        header=(r"\textbf{Method} & \textbf{Instance A: av (min -- max)} & "
                r"\textbf{Instance B: av (min -- max)} \\"),
        caption=(r"Objective function values reported as \texttt{average (min -- max)} across "
                 r"200 runs for Instance A and Instance B. Each average, minimum, and maximum is "
                 r"highlighted independently: the best overall value is bold and the best value "
                 r"within a method category is underlined. NN = nearest neighbor (all positions), "
                 r"GC = greedy cycle, LS = local search."))
    running_time = metric_table(
        tspa, tspb, fields=TIME_FIELDS, decimals=3, separators=separators,
        use_categories=True, label="tab:time_results",
        header=(r"\textbf{Method} & \textbf{Instance A [ms]: av (min -- max)} & "
                r"\textbf{Instance B [ms]: av (min -- max)} \\"),
        caption=(r"Running times in milliseconds reported as \texttt{average (min -- max)} per "
                 r"single run across 200 runs for Instance A and Instance B. Each available "
                 r"average, minimum, and maximum is highlighted independently: the best overall "
                 r"value is bold and the best value within a method category is underlined."))
    local_only = lambda rows: [row for row in rows if row.category == "local_search"]
    return "\n\n".join([
        objective, running_time,
        solution_section("TSPA", local_only(tspa)),
        solution_section("TSPB", local_only(tspb)),
    ]) + "\n"


def task4_latex(results_dir: Path) -> str:
    tspa, tspb = (task4_rows(results_dir, dataset) for dataset in DATASETS)
    separators = (1,)  # after the baseline
    objective = metric_table(
        tspa, tspb, fields=OBJECTIVE_FIELDS, decimals=0, separators=separators,
        use_categories=False, label="tab:task4_obj",
        header=(r"\textbf{Method} & \textbf{Instance A: av (min -- max)} & "
                r"\textbf{Instance B: av (min -- max)} \\"),
        caption=(r"Objective function values reported as \texttt{average (min -- max)} across "
                 r"200 runs from the same random starting solutions. The best value of each "
                 r"average, minimum, and maximum is bold. $k$ = number of candidate edges per "
                 r"node."))
    running_time = metric_table(
        tspa, tspb, fields=TIME_FIELDS, decimals=3, separators=separators,
        use_categories=False, label="tab:task4_time",
        header=(r"\textbf{Method} & \textbf{Instance A [ms]: av (min -- max)} & "
                r"\textbf{Instance B [ms]: av (min -- max)} \\"),
        caption=(r"Running times in milliseconds reported as \texttt{average (min -- max)} per "
                 r"single run across 200 runs. The best value of each average, minimum, and "
                 r"maximum is bold. Building the candidate lists is a one-off cost of at most "
                 f"{longest_list_time(tspa + tspb):.2f} ms per instance and is not included."))
    moves = moves_table(
        tspa, tspb, label="tab:task4_moves",
        caption=(r"Average number of evaluated and applied moves per run (200 runs). The "
                 r"baseline evaluates the whole neighborhood in every iteration; the candidate "
                 r"variants evaluate only moves that introduce a candidate edge."))
    return "\n\n".join([
        objective, running_time, moves,
        solution_section("TSPA", tspa),
        solution_section("TSPB", tspb),
    ]) + "\n"


def task5_latex(results_dir: Path) -> str:
    tspa, tspb = (task5_rows(results_dir, dataset) for dataset in DATASETS)
    separators = (1,)
    objective = metric_table(
        tspa, tspb, fields=OBJECTIVE_FIELDS, decimals=0, separators=separators,
        use_categories=False, label="tab:task5_obj",
        header=(r"\textbf{Method} & \textbf{Instance A: av (min -- max)} & "
                r"\textbf{Instance B: av (min -- max)} \\"),
        caption=(r"Objective function values reported as \texttt{average (min -- max)} across "
                 r"200 paired random starts. The baseline uses the complete steepest "
                 r"neighborhood; the other methods reuse improving move evaluations."))
    running_time = metric_table(
        tspa, tspb, fields=TIME_FIELDS, decimals=3, separators=separators,
        use_categories=False, label="tab:task5_time",
        header=(r"\textbf{Method} & \textbf{Instance A [ms]: av (min -- max)} & "
                r"\textbf{Instance B [ms]: av (min -- max)} \\"),
        caption=(r"Running times in milliseconds per single run across 200 paired random "
                 r"starts. $k$ denotes the candidate-list size."))
    moves = task5_moves_table(
        tspa, tspb, label="tab:task5_moves",
        caption=(r"Average move-list operations per run across 200 paired random starts. "
                 r"Evaluated counts newly calculated deltas, added counts improving moves "
                 r"inserted into LM, and applied counts accepted moves."))
    return "\n\n".join([
        objective, running_time, moves,
        solution_section("TSPA", tspa),
        solution_section("TSPB", tspb),
    ]) + "\n"


def task7_iterations_table(results_dir: Path) -> str:
    iterations_a = read_task7_iterations(
        report_file(results_dir, 7, "TSPA", "iterations"))
    iterations_b = read_task7_iterations(
        report_file(results_dir, 7, "TSPB", "iterations"))
    heuristic_iterations_a = read_task7_iterations(
        report_file(results_dir, 7, "TSPA", "heuristic-iterations"))
    heuristic_iterations_b = read_task7_iterations(
        report_file(results_dir, 7, "TSPB", "heuristic-iterations"))
    lns_names = TASK7_METHOD_NAMES + TASK7_HEURISTIC_NAMES
    body = []
    for index, name in enumerate(lns_names):
        if index in (3, 6):
            body.append(r"\cmidrule{1-3}")
        if name.startswith("Optional"):
            average_a, minimum_a, maximum_a = heuristic_iterations_a[name]
            average_b, minimum_b, maximum_b = heuristic_iterations_b[name]
        else:
            average_a, minimum_a, maximum_a = iterations_a[name]
            average_b, minimum_b, maximum_b = iterations_b[name]
        body.append(
            f"{name} & {average_a:.1f} ({minimum_a} -- {maximum_a}) & "
            f"{average_b:.1f} ({minimum_b} -- {maximum_b}) \\\\")
    header = (
        r"\textbf{LNS variant} & \textbf{Instance A: av (min -- max)} & "
        r"\textbf{Instance B: av (min -- max)} \\")
    return table_lines(
        r"Number of main-loop iterations of the LNS variants.",
        "tab:task7_iterations",
        r"p{8.3cm} X X",
        header,
        body)


def task7_latex(results_dir: Path) -> str:
    tspa, tspb = (task7_rows(results_dir, dataset) for dataset in DATASETS)
    separators = (3, 6)
    objective = metric_table(
        tspa, tspb, fields=OBJECTIVE_FIELDS, decimals=0, separators=separators,
        use_categories=True, label="tab:task7_obj",
        header=(r"\textbf{Method} & \textbf{Instance A: av (min -- max)} & "
                r"\textbf{Instance B: av (min -- max)} \\"),
        caption=(r"Objective function values reported as \texttt{average (min -- max)} "
                 r"across 20 runs. The LNS variants were stopped after the average running "
                 r"time of MSLS (400 steepest local searches)."))
    running_time = metric_table(
        tspa, tspb, fields=TIME_FIELDS, decimals=3, separators=separators,
        use_categories=True, label="tab:task7_time",
        header=(r"\textbf{Method} & \textbf{Instance A [ms]: av (min -- max)} & "
                r"\textbf{Instance B [ms]: av (min -- max)} \\"),
        caption=(r"Running times in milliseconds reported as \texttt{average (min -- max)} "
                 r"per single run across 20 runs."))
    iterations = task7_iterations_table(results_dir)
    try:
        solution_tspa = tspa + task7_subpath_sweep_rows(results_dir, "TSPA")
        solution_tspb = tspb + task7_subpath_sweep_rows(results_dir, "TSPB")
    except (FileNotFoundError, KeyError):
        solution_tspa = tspa
        solution_tspb = tspb
    sections = [
        objective, running_time, iterations,
        solution_section("TSPA", solution_tspa),
        solution_section("TSPB", solution_tspb),
    ]

    try:
        sections.append(task7_subpath_sweep_latex(results_dir))
    except (FileNotFoundError, KeyError):
        pass
    return "\n\n".join(sections) + "\n"


def task7_heuristic_latex(results_dir: Path) -> str:
    rows_by_dataset = {}
    for dataset in DATASETS:
        report = read_report(report_file(results_dir, 7, dataset, "heuristic"))
        solutions = read_combined_solutions(
            report_file(results_dir, 7, dataset, "heuristic")
            .with_name(f"task7-solution-{dataset.lower()}-heuristic.txt"))
        rows = []
        for name in TASK7_HEURISTIC_NAMES:
            cost, best = solutions[name]
            rows.append(Row(name, "heuristic", replace(
                report[name], minimum=cost, best_solution=best)))
        rows_by_dataset[dataset] = rows

    tspa = rows_by_dataset["TSPA"]
    tspb = rows_by_dataset["TSPB"]
    objective = metric_table(
        tspa, tspb, fields=OBJECTIVE_FIELDS, decimals=0, separators=(),
        use_categories=False, label="tab:task7_heuristic_obj",
        header=(r"\textbf{Optional heuristic method} & "
                r"\textbf{Instance A: av (min -- max)} & "
                r"\textbf{Instance B: av (min -- max)} \\"),
        caption=(r"Optional separate experiment for randomized bad-node removal "
                 r"(30 nodes). Probability is weighted by node cost and the maximum "
                 r"length of the two adjacent cycle edges."))
    running_time = metric_table(
        tspa, tspb, fields=TIME_FIELDS, decimals=3, separators=(),
        use_categories=False, label="tab:task7_heuristic_time",
        header=(r"\textbf{Optional heuristic method} & "
                r"\textbf{Instance A [ms]: av (min -- max)} & "
                r"\textbf{Instance B [ms]: av (min -- max)} \\"),
        caption=(r"Running times for the optional randomized bad-node LNS experiment."))

    iteration_rows = []
    for dataset in DATASETS:
        iterations = read_task7_iterations(
            report_file(results_dir, 7, dataset, "heuristic-iterations"))
        iteration_rows.append((dataset, iterations))
    body = []
    for name in TASK7_HEURISTIC_NAMES:
        a = iteration_rows[0][1][name]
        b = iteration_rows[1][1][name]
        body.append(
            f"{name} & {a[0]:.1f} ({a[1]} -- {a[2]}) & "
            f"{b[0]:.1f} ({b[1]} -- {b[2]}) \\\\")
    iterations = table_lines(
        r"Number of iterations in the optional randomized bad-node LNS experiment.",
        "tab:task7_heuristic_iterations",
        r"p{8.3cm} X X",
        (r"\textbf{Optional heuristic method} & "
         r"\textbf{Instance A: av (min -- max)} & "
         r"\textbf{Instance B: av (min -- max)} \\"),
        body)
    return "\n\n".join([
        objective, running_time, iterations,
        solution_section("TSPA", tspa),
        solution_section("TSPB", tspb),
    ])


def task7_subpath_sweep_names() -> list[str]:
    return [
        f"LNS with LS - 3-5 subpaths ({percentage}% removed)"
        for percentage in range(10, 71, 10)
    ]


def task7_subpath_sweep_rows(results_dir: Path, dataset: str) -> list[Row]:
    names = task7_subpath_sweep_names()
    report = read_report(
        report_file(results_dir, 7, dataset, "subpath-sweep"))
    solutions = read_combined_solutions(
        report_file(results_dir, 7, dataset, "subpath-sweep")
        .with_name(f"task7-solution-{dataset.lower()}-subpath-sweep.txt"))
    rows = []
    for name in names:
        cost, best = solutions[name]
        rows.append(Row(name, "lns_subpath_sweep", replace(
            report[name], minimum=cost, best_solution=best)))
    return rows


def task7_subpath_sweep_latex(results_dir: Path) -> str:
    names = [
        *task7_subpath_sweep_names()
    ]
    tspa = task7_subpath_sweep_rows(results_dir, "TSPA")
    tspb = task7_subpath_sweep_rows(results_dir, "TSPB")
    objective = metric_table(
        tspa, tspb, fields=OBJECTIVE_FIELDS, decimals=0, separators=(),
        use_categories=False, label="tab:task7_subpath_sweep_obj",
        header=(r"\textbf{LNS method} & "
                r"\textbf{Instance A: av (min -- max)} & "
                r"\textbf{Instance B: av (min -- max)} \\"),
        caption=(r"Separate experiment with local search and 3--5 random subpaths "
                 r"removed at 10\%--70\% of the 100-node cycle."))
    running_time = metric_table(
        tspa, tspb, fields=TIME_FIELDS, decimals=3, separators=(),
        use_categories=False, label="tab:task7_subpath_sweep_time",
        header=(r"\textbf{LNS method} & "
                r"\textbf{Instance A [ms]: av (min -- max)} & "
                r"\textbf{Instance B [ms]: av (min -- max)} \\"),
        caption=(r"Running times for the 3--5-subpath removal percentage sweep."))

    iteration_rows = []
    for dataset in DATASETS:
        iterations = read_task7_iterations(
            report_file(results_dir, 7, dataset, "subpath-sweep-iterations"))
        iteration_rows.append(iterations)
    body = []
    for name in names:
        a = iteration_rows[0][name]
        b = iteration_rows[1][name]
        body.append(
            f"{name} & {a[0]:.1f} ({a[1]} -- {a[2]}) & "
            f"{b[0]:.1f} ({b[1]} -- {b[2]}) \\\\")
    iterations = table_lines(
        r"Number of iterations in the 3--5-subpath removal percentage sweep.",
        "tab:task7_subpath_sweep_iterations",
        r"p{8.3cm} X X",
        (r"\textbf{LNS method} & "
         r"\textbf{Instance A: av (min -- max)} & "
         r"\textbf{Instance B: av (min -- max)} \\"),
        body)
    return "\n\n".join([
        objective, running_time, iterations,
        solution_section("TSPA", tspa),
        solution_section("TSPB", tspb),
    ])


def longest_list_time(rows: list[Row]) -> float:
    return max((row.result.lists_ms for row in rows if row.result.lists_ms is not None),
               default=0.0)


GENERATORS = {"3": task3_latex, "4": task4_latex, "5": task5_latex,
              "7": task7_latex}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--results-dir", type=Path, default=Path("calculated_solutions"))
    parser.add_argument("--task", choices=["3", "4", "5", "7", "all"], default="all")
    args = parser.parse_args()

    tasks = list(GENERATORS) if args.task == "all" else [args.task]
    for task in tasks:
        try:
            output = GENERATORS[task](args.results_dir)
        except (FileNotFoundError, KeyError) as error:
            print(f"Task {task}: skipped, missing input ({error})")
            continue
        path = args.results_dir / f"task{task}" / f"task{task}-latex.tex"
        path.write_text(output)
        print(output, end="")
        print(f"\nWrote LaTeX to {path}\n", flush=True)


if __name__ == "__main__":
    main()