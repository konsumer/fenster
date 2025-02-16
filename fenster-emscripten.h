// this is an emscripten-only implementation of fenster

#include <stdint.h>
#include <stdlib.h>
#include "emscripten.h"

struct fenster {
  const int width;
  const int height;
  uint32_t *buf;
  uint8_t keys[256]; /* keys are mostly ASCII, but arrows are 17..20 */
  int mod;       /* mod is 4 bits mask, ctrl=1, shift=2, alt=4, meta=8 */
  int x;
  int y;
  int mouse;
  const char *title; // I put it at the end, so everything else is fixed-length
};

// gives mem out-of-bounds error in emscripten, for some reason
#define fenster_pixel(f, x, y) ((f)->buf[((y) * (f)->width) + (x)])

static bool running = true;

EM_JS(int, fenster_open, (struct fenster *f), {
  if (!Module.canvas) {
    Module.canvas = document.getElementById('canvas');
    if (!Module.canvas) {
      document.createElement("canvas");
      document.appendElement(Module.canvas);
    }
  }
  const width = Module.HEAP32[f/4];
  const height = Module.HEAP32[(f/4) + 1];
  Module.canvas.width = width;
  Module.canvas.height = height;
  Module.ctx = canvas.getContext("2d");
  Module.screen = Module.ctx.getImageData(0, 0, width, height);

  // TODO: handle mod

  Module.canvas.addEventListener('mousemove', e => {
    Module.HEAP32[(f/4) + 5] = e.offsetX;
    Module.HEAP32[(f/4) + 6] = e.offsetX;
  });

  Module.canvas.addEventListener('mousedown', e => {
    Module.HEAP32[(f/4) + 7] = e.which;
  });

  Module.canvas.addEventListener('mouseup', e => {
    Module.HEAP32[(f/4) + 7] = 0;
  });

  Module.canvas.addEventListener('keydown', e => {
    Module.HEAP32[ Module.HEAP32[(f/4) + 3] + e.keyCode] = 1;
  });

  Module.canvas.addEventListener('keyup', e => {
    Module.HEAP32[ Module.HEAP32[(f/4) + 3] + e.keyCode] = 0;
  });
});

EM_JS(void, emscripten_fenster_loop, (struct fenster *f), {
  // set alpha to 100%
  const pmax = f + 8 + Module.canvas.width * Module.canvas.height * 4;
  for (let p=f+8;p < pmax;p+=4) {
    Module.HEAPU8[p+3] = 0xFF;
  }
  const bufSize = Module.canvas.width*Module.canvas.height*4;
  Module.screen.data.set(Module.HEAPU8.slice(f+8, f+8+bufSize));
  Module.ctx.putImageData(Module.screen, 0, 0);
});

int fenster_loop(struct fenster *f) {
  if (running) {
    emscripten_fenster_loop(f);
  }

  // this is hack to make it not pin the main-loop (~60fps)
  emscripten_sleep(16);

  return running ? 0 : 1;
}

void fenster_close(struct fenster *f) {
  running = false;
}

void fenster_sleep(int64_t ms) {
  emscripten_sleep(ms);
}

EM_JS(int64_t, fenster_time, (void), {
  return (new Date()).time;
});
