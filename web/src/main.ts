import './style.css';
import { environmentLabel, quotaDescription } from './presentation';
import { chartSvg, mobileStrategy } from './visualizations';
import data from '../data/benchmark.json';
import report from '../data/report.json';
import { derive, format, type Metric } from './metrics';
const rows = derive(data.rows);
const fastest = rows.reduce((a, b) => a.seconds < b.seconds ? a : b);
const m = data.machine;
const rawUrl = new URL('../data/benchmark.json', import.meta.url).href;
document.querySelector<HTMLDivElement>('#app')!.innerHTML = `
<header>
<a class="brand" href="#">P<span>∥</span>THREAD <small>FILE ANALYZER</small>
</a>
<nav aria-label="Main navigation">
<a href="#results">Results</a>
<a href="#strategy">How it works</a>
<a href="#method">Method</a>
</nav>
<a class="source" href="https://github.com/AliHamzaAzam/PThread-FileAnalyzer">Source code</a>
</header>
<main id="main">
<section class="hero">
<p class="eyebrow">A STUDY IN PARALLEL PROCESSING</p>
<h1>One file.<br>Many threads.<br>
<span>Measured results.</span>
</h1>
<div class="intro">
<p>Divide the work. Keep the evidence.</p>
<p>A C++ file analyzer that uses memory mapping and POSIX threads to turn graph, text, log and matrix files into useful statistics.</p>
<a class="button" href="#results">Explore the measurements ↓</a>
</div>
</section>
<section class="overview" aria-label="Benchmark overview">
<div>
<span>DATASET</span>
<strong>${m.dataset.edges.toLocaleString("en-US")} edges</strong>
<small>${m.dataset.name} · ${(m.dataset.bytes / 1e6).toFixed(2)} MB</small>
</div>
<div>
<span>BEST MEDIAN IN THIS RUN</span>
<strong>${format(fastest.speedup, 'speedup')} speedup</strong>
<small>${fastest.threads} threads · ${format(fastest.runtime, 'runtime')}</small>
</div>
<div>
<span>BENCHMARK ENVIRONMENT</span>
<strong>${environmentLabel(m)}</strong>
<small>Affinity ${m.affinity ? "requested" : "off"} · ${m.availableCpus} allowed CPUs</small>
</div>
</section>
<section id="results">
<div class="section-head">
<div>
<p class="eyebrow">01 / THE MEASUREMENTS</p>
<h2>What changes with more threads?</h2>
</div>
<span class="pill">TASK 1 · SYNTHETIC GRAPH</span>
</div>
<p class="section-copy">Each point is the median of five runs on the same input. Speedup compares with one thread; efficiency divides that speedup by the number of threads. This small warm-cache test is not an Orkut benchmark.</p>
<div class="chart-card">
<fieldset>
<legend>Choose a chart</legend>
<label>
<input type="radio" name="metric" value="runtime" checked>Runtime</label>
<label>
<input type="radio" name="metric" value="speedup">Speedup</label>
<label>
<input type="radio" name="metric" value="efficiency">Efficiency</label>
</fieldset>
<div id="chart" aria-live="polite">
</div>
<p id="chart-note" class="muted">
</p>
</div>
<details>
<summary>View all recorded measurements</summary>
<div class="table-scroll" role="region" aria-label="All benchmark measurements" tabindex="0">
<table>
<caption>Linux synthetic graph, five runs per thread count</caption>
<thead>
<tr>
<th scope="col">Threads</th>
<th scope="col">Median</th>
<th scope="col">Min to max</th>
<th scope="col">Speedup</th>
<th scope="col">Efficiency</th>
</tr>
</thead>
<tbody>${rows.map(r => `<tr>
<th scope="row">${r.threads}</th>
<td>${format(r.runtime, 'runtime')}</td>
<td>${(r.min * 1000).toFixed(1)} to ${(r.max * 1000).toFixed(1)} ms</td>
<td>${format(r.speedup, 'speedup')}</td>
<td>${format(r.efficiency, 'efficiency')}</td>
</tr>`).join('')}</tbody>
</table>
</div>
</details>
<p class="callout">More threads do not guarantee proportional gains. This run had ${m.availableCpus} allowed CPUs and ${quotaDescription(m.cpuQuota)} from its cgroup quota. Scheduling, mutex merges and noise can affect these short runs; the measurements do not isolate their effects.</p>
</section>
<section id="strategy">
<p class="eyebrow">02 / THE STRATEGY</p>
<h2>Split locally. Combine once.</h2>
<p class="section-copy">Graph, text and log analyzers divide a memory-mapped file near equal byte offsets, then move boundaries to a newline. Workers build local counts and merge them under a mutex before returning.</p>
<div class="diagram">${mobileStrategy}<svg class="flow-desktop" viewBox="0 0 850 230" role="img" aria-labelledby="flow-title flow-desc">
<title id="flow-title">File processing with four illustrative workers</title>
<desc id="flow-desc">Map a file, align four chunks to newline boundaries, process local statistics in parallel, merge under a mutex, then join and summarize.</desc>
<g fill="none" stroke="currentColor" stroke-width="2">
<path d="M160 115H210M210 35V195M210 35H260M210 88H260M210 142H260M210 195H260M420 35H470M420 88H470M420 142H470M420 195H470M470 35V195M470 115H520M680 115H720"/>
</g>
<g class="node">
<rect x="0" y="80" width="160" height="70" rx="8"/>
<rect x="260" y="10" width="160" height="46" rx="8"/>
<rect x="260" y="65" width="160" height="46" rx="8"/>
<rect x="260" y="119" width="160" height="46" rx="8"/>
<rect x="260" y="172" width="160" height="46" rx="8"/>
<rect x="520" y="80" width="160" height="70" rx="8"/>
<rect x="720" y="80" width="128" height="70" rx="8"/>
</g>
<g text-anchor="middle">
<text x="80" y="111">Map file</text>
<text x="80" y="133" class="sub">mmap</text>
<text x="340" y="39">Worker 1 · local</text>
<text x="340" y="94">Worker 2 · local</text>
<text x="340" y="148">Worker 3 · local</text>
<text x="340" y="201">Worker 4 · local</text>
<text x="600" y="111">Merge</text>
<text x="600" y="133" class="sub">mutex protected</text>
<text x="784" y="111">Join</text>
<text x="784" y="133" class="sub">summarize</text>
</g>
</svg>
</div>
<div class="workloads">
<article>
<span>01</span>
<h3>Graph</h3>
<p>Count edges, node degrees and the ten highest-degree nodes.</p>
</article>
<article>
<span>02 / 03</span>
<h3>Text</h3>
<p>Count words, frequencies and words starting with a vowel.</p>
</article>
<article>
<span>04</span>
<h3>Logs</h3>
<p>Count entries by level and block-specific errors.</p>
</article>
<article>
<span>05</span>
<h3>Matrix</h3>
<p>Partition NumPy rows; merge sums, extrema and row/column totals after joining.</p>
</article>
</div>
</section>
<section id="method">
<p class="eyebrow">03 / THE EVIDENCE</p>
<h2>Reproducible, with boundaries.</h2>
<div class="method-grid">
<article>
<h3>Recorded benchmark</h3>
<dl>
<dt>Recorded</dt>
<dd>${m.date}</dd>
<dt>Machine</dt>
<dd>${m.cpu}</dd>
<dt>Platform</dt>
<dd>${environmentLabel(m)} · kernel ${m.kernel}</dd>
<dt>Compiler</dt>
<dd>${m.compiler}</dd>
<dt>Flags</dt>
<dd>
<code>${m.flags.join(' ')}</code>
</dd>
<dt>Input</dt>
<dd>1,000,000 generated edges over 10,000 nodes; every run checks both totals.</dd>
<dt>Protocol</dt>
<dd>One warmup, then five repetitions for each of 1, 2, 4 and 8 threads. Fixed ascending order, no cache clearing, affinity disabled.</dd>
</dl>
<p>The compiler's forced cstring include supplies the existing source's missing declaration for memchr; C++ files are unchanged. Runtime is the program's internal Execution Time, excluding file mapping, chunk setup and printing.</p>
<a href="${rawUrl}" download="benchmark.json">Download measurements and metadata ↓</a>
</article>
<article>
<h3>Original macOS report</h3>
<p>The February 2025 report describes an Apple M3 Pro with 11 cores and 18 GB memory. Its CPU-profile observations are separate evidence, not points in the Linux chart.</p>
<div class="table-scroll" role="region" aria-label="Original report measurements" tabindex="0">
<table>
<caption>Reported profile duration in seconds, affinity on / off</caption>
<thead>
<tr>
<th scope="col">Workload</th>
<th scope="col">On</th>
<th scope="col">Off</th>
<th scope="col">Page</th>
</tr>
</thead>
<tbody>${report.rows.map(r => `<tr>
<th scope="row">${r.workload}</th>
<td>${r.affinityOn.toFixed(3)}</td>
<td>${r.affinityOff.toFixed(3)}</td>
<td>${r.page}</td>
</tr>`).join('')}</tbody>
</table>
</div>
<p>The profiling script uses 10 threads. No raw CSVs are checked in for the report's scaling plots, so those plots have not been digitized. These profile durations cannot establish single-thread speedup or predict Linux performance. The report does not record exact compiler flags, OS version or dataset hashes. macOS affinity tags are scheduling hints, not verified core pinning.</p>
<a href="https://github.com/AliHamzaAzam/PThread-FileAnalyzer/blob/main/i222126_A_Report.pdf">Read the original report ↗</a>
</article>
</div>
</section>
<footer>
<span>PThread File Analyzer</span>
<p>Measured on a small synthetic workload. No claims about production performance.</p>
<a href="#">Back to top ↑</a>
</footer>
</main>`;
const chart = document.querySelector<HTMLDivElement>('#chart')!;
let selectedMetric: Metric = 'runtime';
function render(metric: Metric) {
  selectedMetric = metric;
  chart.innerHTML = chartSvg(rows, metric, Math.floor(chart.clientWidth));
  document.querySelector('#chart-note')!.textContent = metric === 'runtime' ? 'Lower is faster. Median of five runs; min and max are available in the table.' : metric === 'speedup' ? 'Higher is faster. Speedup = one-thread median / current median.' : 'Efficiency = speedup / thread count × 100. This is not measured CPU utilization.';
}
document.querySelectorAll<HTMLInputElement>('input[name="metric"]').forEach(input => input.addEventListener('change', () => render(input.value as Metric)));
render('runtime');
new ResizeObserver(() => render(selectedMetric)).observe(chart);
