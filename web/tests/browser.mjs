import { createRequire } from 'node:module';
import assert from 'node:assert/strict';
const require = createRequire(import.meta.url);
const { chromium } = require(process.env.PLAYWRIGHT_MODULE || 'playwright');
const browser = await chromium.launch({ executablePath: process.env.CHROME_PATH || '/usr/bin/chromium', headless: true, args: ['--no-sandbox'] });
try {
  for (const width of [1280, 768, 390, 320]) {
    const page = await browser.newPage({ viewport: { width, height: 900 } });
    const errors = [];
    page.setDefaultTimeout(10000);
    page.on('pageerror', error => errors.push(error.message));
    page.on('console', message => { if (message.type() === 'error') errors.push(message.text()); });
    await page.goto(process.env.SITE_URL || 'http://127.0.0.1:4175', { waitUntil: 'networkidle' });
    assert.equal(await page.title(), 'PThread File Analyzer | Results');
    assert.equal(await page.locator('body').evaluate(el => el.scrollWidth <= window.innerWidth), true);
    for (const [name, title] of [['Speedup', 'Speedup (×) by thread count'], ['Efficiency', 'Efficiency (%) by thread count'], ['Runtime', 'Runtime (ms) by thread count']]) {
      await page.getByRole('radio', { name, exact: true }).check();
      assert.equal(await page.locator('#chart-title').textContent(), title);
      const smallestLabel = await page.locator('#chart text').evaluateAll(nodes => Math.min(...nodes.map(node => Number.parseFloat(getComputedStyle(node).fontSize) * node.getScreenCTM().a)));
      assert.ok(smallestLabel >= 12, `Chart labels must remain legible at ${width}px, got ${smallestLabel}px`);
    }
    const flowLabels = await page.locator('.diagram svg:visible text').evaluateAll(nodes => nodes.map(node => Number.parseFloat(getComputedStyle(node).fontSize) * node.getScreenCTM().a));
    assert.ok(flowLabels.length > 0 && Math.min(...flowLabels) >= 12, `Strategy labels must remain legible at ${width}px`);
    await page.locator('summary').click();
    assert.equal(await page.locator('details').getAttribute('open'), '');
    assert.equal(await page.locator('details tbody tr').count(), 4);
    const downloaded = await page.locator('a[download="benchmark.json"]').evaluate(async link => {
      const response = await fetch(link.href);
      if (!response.ok) throw new Error(`Download failed: ${response.status}`);
      return response.json();
    });
    assert.equal(downloaded.rows.length, 4);
    assert.equal(await page.locator('.overview').innerText().then(text => text.includes(`${downloaded.machine.platform} · ${downloaded.machine.architecture}`)), true);
    await page.getByRole('link', { name: 'How it works', exact: true }).click();
    assert.equal(new URL(page.url()).hash, '#strategy');
    // Resizing must preserve the selected metric and its accessible description.
    await page.getByRole('radio', { name: 'Efficiency', exact: true }).check();
    await page.setViewportSize({ width: width === 390 ? 1280 : 390, height: 900 });
    await page.waitForTimeout(100);
    assert.equal(await page.locator('#chart-title').textContent(), 'Efficiency (%) by thread count');
    await page.setViewportSize({ width, height: 900 });
    await page.getByRole('radio', { name: 'Runtime', exact: true }).check();
    assert.deepEqual(errors, []);
    await page.screenshot({ path: `/tmp/c12-final-${width}.png`, fullPage: true });
    await page.close();
    console.log(`PASS served Chromium: ${width}px, no overflow or console errors, all chart controls and table`);
  }
} finally { await browser.close(); }
