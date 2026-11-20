/* SPDX-License-Identifier: GPL-3.0-only */
/* AudioWorklet: kosmar-fm1 WASM tick + 30 fps framebuffer posts. */

const FS = 44100;

class KosmarFm1Processor extends AudioWorkletProcessor {
  constructor() {
    super();
    this.ex = null;
    this.mem = null;
    this.readPos = 0;
    this.frameAcc = 0;
    this.sentWrites = 0;
    this.port.onmessage = (ev) => {
      void this.onMsg(ev.data).catch((err) => {
        const msg = err && err.message ? err.message : String(err);
        this.port.postMessage({ type: "error", step: "worklet-load", message: msg });
        console.error("[kosmar-fm1 worklet]", err);
      });
    };
  }

  async onMsg(m) {
    if (!m || !m.type)
      return;
    if (m.type === "load") {
      try {
        const { wasm, base, store } = m;
        if (!wasm || !(wasm instanceof ArrayBuffer) || wasm.byteLength < 8)
          throw new Error(`invalid WASM buffer (${wasm && wasm.byteLength})`);
        const imports = {
          env: new Proxy({ memory: new WebAssembly.Memory({ initial: 256, maximum: 512 }) }, {
            get: (t, k) => (k in t ? t[k] : () => 0)
          }),
          wasi_snapshot_preview1: new Proxy({}, { get: () => () => 0 })
        };
        const { instance } = await WebAssembly.instantiate(wasm, imports);
        const ex = instance.exports;
        this.mem = ex.memory || imports.env.memory;
        if (ex._initialize)
          ex._initialize();
        this.mem = ex.memory || this.mem;
        if (store && ex.web_store_ptr) {
          if (store[0]) {
            const src = new Uint8Array(store[0]);
            const cap = ex.web_store_len ? ex.web_store_len() : 260;
            const dst = new Uint8Array(this.mem.buffer, ex.web_store_ptr(), cap);
            dst.fill(0);
            dst.set(src.subarray(0, Math.min(src.length, cap)));
          }
        } else if (store && ex.web_store && ex.web_store_max) {
          const max = ex.web_store_max();
          for (const [obj, bytes] of Object.entries(store)) {
            const b = new Uint8Array(bytes).subarray(0, max);
            new Uint8Array(this.mem.buffer, ex.web_store(+obj), b.length).set(b);
            ex.web_store_set_len(+obj, b.length);
          }
        }
        if (ex.web_master && m.master !== undefined)
          ex.web_master(m.master >>> 0);
        if (ex.web_boot) {
          let ms0 = Date.now() >>> 0;
          try {
            if (typeof performance !== "undefined" && performance.now)
              ms0 = Math.floor(performance.now()) >>> 0;
            const cr = globalThis.crypto;
            if (cr && cr.getRandomValues) {
              const rnd = new Uint32Array(2);
              cr.getRandomValues(rnd);
              ms0 = (ms0 ^ rnd[0] ^ (rnd[1] << 1)) >>> 0;
            }
          } catch (_) { /* worklet may lack crypto */ }
          ex.web_boot(ms0);
        }
        this.sentWrites = ex.web_store_writes ? ex.web_store_writes() : 0;
        this.readPos = ex.web_out_w ? ex.web_out_w() : 0;
        this.ex = ex;
        this.port.postMessage({ type: "ready", base });
      } catch (err) {
        const msg = err && err.message ? err.message : String(err);
        this.port.postMessage({ type: "error", step: "worklet-load", message: msg });
        console.error("[kosmar-fm1 worklet] load failed:", err);
      }
      return;
    }
    if (!this.ex)
      return;
    switch (m.type) {
      case "buttons":
        this.ex.web_buttons(m.mask >>> 0);
        break;
      case "keys":
        this.ex.web_keys(m.mask >>> 0);
        break;
      case "enc":
        this.ex.web_enc(m.role | 0, (m.delta ?? m.n ?? 0) | 0);
        break;
      case "master":
        this.ex.web_master((m.val ?? m.value ?? 2800) >>> 0);
        break;
      case "midi":
        if (this.ex.web_midi_in)
          this.ex.web_midi_in(m.st >>> 0, m.d1 >>> 0, m.d2 >>> 0);
        break;
      case "flushStore":
        this.postStoreIfDirty();
        break;
      default:
        break;
    }
  }

  postStoreIfDirty() {
    if (!this.ex || !this.ex.web_store_writes || !this.ex.web_store_ptr)
      return;
    if (this.ex.web_store_writes() === this.sentWrites)
      return;
    this.sentWrites = this.ex.web_store_writes();
    const cap = this.ex.web_store_len ? this.ex.web_store_len() : 260;
    const n = this.ex.web_store_bytes ? this.ex.web_store_bytes() : cap;
    const len = n > 0 && n <= cap ? n : cap;
    this.port.postMessage({
      type: "store",
      store: { 0: new Uint8Array(this.mem.buffer, this.ex.web_store_ptr(), len).slice() },
    });
  }

  process(_inputs, outputs) {
    const out = outputs[0];
    if (!out || !out[0])
      return true;
    const chL = out[0];
    const chR = out[1] || out[0];
    const n = chL.length;

    if (this.ex) {
      if (this.ex.web_out_w) {
        for (let guard = 0; guard < 512 && ((this.ex.web_out_w() - this.readPos) >>> 0) < n; guard++)
          this.ex.web_render(8);

        const lBase = this.ex.web_out_l() / 4;
        const rBase = this.ex.web_out_r() / 4;
        const f32 = new Float32Array(this.mem.buffer);

        for (let i = 0; i < n; i++) {
          chL[i] = f32[lBase + (this.readPos & 65535)];
          chR[i] = f32[rBase + (this.readPos & 65535)];
          this.readPos++;
        }
        this.postStoreIfDirty();
      } else {
        this.ex.web_render(n);
        chL.set(new Float32Array(this.mem.buffer, this.ex.web_out_l(), n));
        chR.set(new Float32Array(this.mem.buffer, this.ex.web_out_r(), n));
      }

      this.frameAcc += n;
      if (this.frameAcc >= FS / 30) {
        this.frameAcc -= FS / 30;
        const fbPtr = this.ex.web_fb() / 2;
        const u16 = new Uint16Array(this.mem.buffer, fbPtr * 2, 240 * 240);
        const fbCopy = u16.slice();
        const msg = {
          type: "frame",
          buttons: this.ex.web_lit_buttons(),
          dim: this.ex.web_dim_buttons ? this.ex.web_dim_buttons() : 0,
          keys: this.ex.web_lit_keys(),
          lfoBtn: this.ex.web_btn_level ? this.ex.web_btn_level(5) : 0,
          arpBtn: this.ex.web_btn_level ? this.ex.web_btn_level(10) : 0,
          seqBtn: this.ex.web_btn_level ? this.ex.web_btn_level(11) : 0,
          recBtn: this.ex.web_btn_level ? this.ex.web_btn_level(13) : 0,
          mode: this.ex.web_mode ? this.ex.web_mode() : 0,
          fb: fbCopy,
        };
        this.port.postMessage(msg, [fbCopy.buffer]);
      }
    } else {
      for (let i = 0; i < n; i++) {
        chL[i] = 0;
        chR[i] = 0;
      }
    }
    return true;
  }
}

registerProcessor("kosmar-fm1", KosmarFm1Processor);
