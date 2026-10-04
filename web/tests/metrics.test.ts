import { test } from 'node:test';
import assert from 'node:assert/strict';
import { derive, format } from '../src/metrics.ts';
const row = (threads: number, seconds: number) => ({ threads, seconds, min: seconds, max: seconds, samples: [seconds] });
test('speedup and efficiency use matching single-thread baseline', () => {
  const result = derive([row(4, 2), row(1, 4)]);
  assert.equal(result[0].speedup, 2);
  assert.equal(result[0].efficiency, 50);
  assert.equal(result[0].runtime, 2000);
  assert.equal(result[1].speedup, 1);
});
test('missing baseline, duplicate threads and invalid timings are rejected', () => {
  for (const rows of [[], [row(2, 1)], [row(1, 1), row(1, 2)], [row(1, 0)], [row(1, NaN)], [row(1, 1), row(0, 2)], [row(1, 1), row(2, Infinity)]]) assert.throws(() => derive(rows));
});
test('metric labels state units', () => {
  assert.equal(format(1.234, 'speedup'), '1.23×');
  assert.equal(format(12, 'runtime'), '12.0 ms');
  assert.equal(format(50, 'efficiency'), '50.0%');
});
