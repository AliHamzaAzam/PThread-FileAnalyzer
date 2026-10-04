export type Row = { threads: number; seconds: number; min: number; max: number; samples: number[] };
export type Metric = 'runtime' | 'speedup' | 'efficiency';
export function derive(rows: Row[]) {
  const baseline = rows.find(row => row.threads === 1);
  if (!baseline || !rows.length) throw new Error('A one-thread baseline is required');
  if (new Set(rows.map(row => row.threads)).size !== rows.length) throw new Error('Duplicate thread counts');
  return rows.map(row => {
    if (!Number.isInteger(row.threads) || row.threads < 1 || !Number.isFinite(row.seconds) || row.seconds <= 0) throw new Error('Invalid measurement');
    const speedup = baseline.seconds / row.seconds;
    return { ...row, runtime: row.seconds * 1000, speedup, efficiency: 100 * speedup / row.threads };
  });
}
export function format(value: number, metric: Metric) {
  return `${value.toFixed(metric === 'runtime' ? 1 : metric === 'speedup' ? 2 : 1)}${metric === 'runtime' ? ' ms' : metric === 'speedup' ? '×' : '%'}`;
}
