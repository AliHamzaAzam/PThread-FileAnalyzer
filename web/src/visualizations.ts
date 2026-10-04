import { format, type Metric, type derive } from './metrics';

type Measurements = ReturnType<typeof derive>;

// Match the SVG coordinate width to its container so labels do not shrink on phones.
export function chartSvg(rows: Measurements, metric: Metric, width: number): string {
  const unit = metric === 'runtime' ? 'Runtime (ms)' : metric === 'speedup' ? 'Speedup (×)' : 'Efficiency (%)';
  const maximum = Math.max(...rows.map(row => row[metric]));
  const description = rows.map(row => `${row.threads} threads: ${format(row[metric], metric)}`).join('; ');
  let plot: string;
  if (width < 600) {
    const barWidth = width - 135;
    plot = `<text x="5" y="25" class="axis">Threads</text><text x="${width - 5}" y="25" text-anchor="end" class="axis">${unit}</text>`;
    plot += rows.map((row, index) => {
      const y = 60 + index * 55;
      const length = row[metric] / maximum * barWidth;
      return `<text x="16" y="${y + 18}" class="axis">${row.threads}</text>
        <rect x="48" y="${y}" width="${length}" height="27" rx="3" fill="#087f78"/>
        <text x="${length + 56}" y="${y + 18}" class="value">${format(row[metric], metric)}</text>`;
    }).join('');
    plot += '<text x="48" y="285" class="axis">Bar lengths start at zero.</text>';
  } else {
    const max = maximum * 1.22;
    const y = (value: number) => 250 - value / max * 210;
    const maxThreads = Math.max(...rows.map(row => row.threads));
    const minThreads = Math.min(...rows.map(row => row.threads));
    const x = (threads: number) => 95 + (threads - minThreads) / Math.max(1, maxThreads - minThreads) * (width - 150);
    plot = [0, 1, 2, 3, 4].map(index => `<line x1="80" y1="${y(max * index / 4)}" x2="${width - 30}" y2="${y(max * index / 4)}" stroke="#e0e9e7"/>
      <text x="65" y="${y(max * index / 4) + 5}" text-anchor="end" class="axis">${(max * index / 4).toFixed(metric === 'speedup' ? 1 : 0)}</text>`).join('');
    plot += `<text x="80" y="18" class="axis">${unit}</text><polyline fill="none" stroke="#087f78" stroke-width="3" points="${rows.map(row => `${x(row.threads)},${y(row[metric])}`).join(' ')}"/>`;
    plot += rows.map(row => `<circle cx="${x(row.threads)}" cy="${y(row[metric])}" r="6" fill="#087f78"/>
      <text x="${x(row.threads)}" y="${y(row[metric]) - 15}" text-anchor="middle" class="value">${format(row[metric], metric)}</text>
      <text x="${x(row.threads)}" y="278" text-anchor="middle" class="axis">${row.threads}</text>`).join('');
    plot += `<text x="${width / 2}" y="305" text-anchor="middle" class="axis">Worker threads</text>`;
  }
  return `<svg viewBox="0 0 ${width} 315" role="img" aria-labelledby="chart-title chart-description"><title id="chart-title">${unit} by thread count</title><desc id="chart-description">${description}. Full numeric data follows below.</desc>${plot}</svg>`;
}

export const mobileStrategy = `<svg class="flow-mobile" viewBox="0 0 280 540" role="img" aria-labelledby="mobile-flow-title mobile-flow-description">
  <title id="mobile-flow-title">Parallel file processing</title>
  <desc id="mobile-flow-description">Map the file, align chunks to newlines, process chunks in parallel workers, merge local counts under a mutex, then join and summarize.</desc>
  <g fill="none" stroke="currentColor" stroke-width="2"><path d="M140 66V100M140 166V215M140 281V330M140 396V430"/></g>
  <g class="node"><rect x="30" y="0" width="220" height="66" rx="8"/><rect x="30" y="100" width="220" height="66" rx="8"/><rect x="0" y="215" width="280" height="66" rx="8"/><rect x="30" y="330" width="220" height="66" rx="8"/><rect x="30" y="430" width="220" height="66" rx="8"/></g>
  <g text-anchor="middle"><text x="140" y="27">Map file</text><text x="140" y="49" class="sub">mmap</text><text x="140" y="127">Align chunks</text><text x="140" y="149" class="sub">newline boundaries</text><text x="140" y="242">Workers 1, 2, ... N</text><text x="140" y="264" class="sub">parallel local counts</text><text x="140" y="357">Merge each worker's counts</text><text x="140" y="379" class="sub">mutex protected</text><text x="140" y="457">Join threads</text><text x="140" y="479" class="sub">then summarize</text></g>
</svg>`;
