#!/usr/bin/env node
// SPDX-License-Identifier: GPL-3.0-only
import { createServer } from "node:http";
import { readFileSync, mkdirSync, writeFileSync } from "node:fs";
import { join, extname } from "node:path";
import puppeteer from "puppeteer-core";

const PAGES = process.env.KO_EMU_PAGES || "/Users/kosmar/.cursor/fm1-wt-pages/build/pages";
const OUT_DIR =
  process.env.KO_CARD_OUT || "/Users/kosmar/.cursor/fm1-wt-publish/publish/assets/cards";
const CHROME =
  process.env.CHROME_PATH ||
  "/Applications/Google Chrome.app/Contents/MacOS/Google Chrome";
const SLUGS = (process.env.KO_CARD_APPS || "")
  .split(/[\s,]+/)
  .filter(Boolean);
const ALL = [
  "dub", "house", "heavypsych", "jazz", "idm", "hiphop", "pop", "funk", "techno",
  "bass", "snowcrash", "triphop", "deck", "soul", "jet",
];
const apps = SLUGS.length ? SLUGS : ALL;

const MIME = {
  ".html": "text/html; charset=utf-8",
  ".js": "text/javascript; charset=utf-8",
  ".wasm": "application/wasm",
  ".json": "application/json",
  ".png": "image/png",
  ".svg": "image/svg+xml",
  ".css": "text/css",
  ".woff2": "font/woff2",
  ".fwsc": "application/octet-stream",
};

function staticServer(root) {
  return createServer((req, res) => {
    let p = (req.url || "/").split("?")[0];
    if (p === "/") p = "/index.html";
    try {
      const file = join(root, decodeURIComponent(p));
      const body = readFileSync(file);
      res.writeHead(200, { "Content-Type": MIME[extname(file)] || "application/octet-stream" });
      res.end(body);
    } catch {
      res.writeHead(404);
      res.end("not found");
    }
  });
}

async function capture(slug, base) {
  const browser = await puppeteer.launch({
    executablePath: CHROME,
    headless: true,
    args: ["--autoplay-policy=no-user-gesture-required", "--use-fake-ui-for-media-stream", "--use-fake-device-for-media-stream"],
  });
  try {
    const page = await browser.newPage();
    await page.setViewport({ width: 900, height: 1400, deviceScaleFactor: 1 });
    await page.goto(`${base}/?app=${slug}`, { waitUntil: "networkidle0", timeout: 120000 });
    const go = await page.$("#go");
    if (go) await go.click();
    await page.waitForFunction(() => {
      const s = document.getElementById("start");
      return s && s.hidden;
    }, { timeout: 60000 });
    await new Promise((r) => setTimeout(r, 3500));
  const lcd = await page.$("#lcd");
  if (!lcd) throw new Error("no #lcd");
    const buf = await lcd.screenshot({ type: "png" });
    const out = join(OUT_DIR, `${slug}.png`);
    writeFileSync(out, buf);
    try {
      const { execFileSync } = await import("node:child_process");
      execFileSync("magick", [out, "-filter", "point", "-resize", "240x240!", "PNG:" + out]);
    } catch {
      /* magick optional */
    }
    console.log(`${slug}\t${out}\t${buf.length}`);
  } finally {
    await browser.close();
  }
}

const server = staticServer(PAGES);
await new Promise((resolve) => server.listen(0, "127.0.0.1", resolve));
const port = server.address().port;
const base = `http://127.0.0.1:${port}`;
mkdirSync(OUT_DIR, { recursive: true });
for (const slug of apps) {
  try {
    await capture(slug, base);
  } catch (e) {
    console.error(`${slug}\tFAIL\t${e.message}`);
    process.exitCode = 1;
  }
}
server.close();