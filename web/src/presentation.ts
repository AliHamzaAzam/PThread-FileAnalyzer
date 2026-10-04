export function environmentLabel(machine: { platform: string; architecture: string }): string {
  return `${machine.platform} · ${machine.architecture}`;
}

export function quotaDescription(raw: string): string {
  const [quota, period] = raw.trim().split(/\s+/);
  if (quota === 'max') return 'no CPU time limit recorded';
  const limit = Number(quota) / Number(period);
  return Number.isFinite(limit) && limit > 0 ? `${limit} CPU equivalents` : 'CPU time limit unavailable';
}
