import { test } from 'node:test';
import assert from 'node:assert/strict';
import { environmentLabel, quotaDescription } from '../src/presentation.ts';

test('environment labels describe the recorded machine on x86 and ARM reruns', () => {
  assert.equal(environmentLabel({ platform: 'Linux', architecture: 'aarch64' }), 'Linux · aarch64');
  assert.equal(environmentLabel({ platform: 'Linux', architecture: 'x86_64' }), 'Linux · x86_64');
});

test('quota description uses the recorded limit and period rather than a fixed core count', () => {
  assert.equal(quotaDescription('400000 100000'), '4 CPU equivalents');
  assert.equal(quotaDescription('50000 100000'), '0.5 CPU equivalents');
  assert.equal(quotaDescription('max 100000'), 'no CPU time limit recorded');
  assert.equal(quotaDescription('unknown'), 'CPU time limit unavailable');
  assert.equal(quotaDescription('400000 0'), 'CPU time limit unavailable');
});
