importScripts("./juiced_mod.js");
const juicedReady = createJuiced();

self.onmessage = async function (event) {
  const juiced = await juicedReady;
  const bfjoust = juiced.bfjoust;
  const request = event.data;

  if (request.type === "trace") {
    try {
      const result = bfjoust(
        preproc(request.left),
        preproc(request.right),
        request.cellCount,
        request.switched,
        true,
      );
      const frameSize = request.cellCount + 5;
      const frameCount = result.trace.size();
      const frames = new Int32Array(frameSize * frameCount);
      const heap = juiced.HEAP32;
      const heapU32 = juiced.HEAPU32;
      for (let i = 0; i < frameCount; i++) {
        const frame = result.trace.get(i);
        const vectorOffset = frame.$$.ptr >>> 2;
        const begin = heapU32[vectorOffset];
        const end = heapU32[vectorOffset + 1];
        if (((end - begin) >>> 2) !== frameSize) {
          throw new Error("Unexpected trace frame size");
        }
        frames.set(heap.subarray(begin >>> 2, end >>> 2), i * frameSize);
      }
      self.postMessage({
        type: "trace",
        state: result.result,
        reason: result.reason,
        frameSize,
        frameCount,
        traceBuffer: frames.buffer,
      }, [frames.buffer]);
    } catch (error) {
      self.postMessage({ type: "error", message: error.message });
    }
    return;
  }

  let count = 0;
  for (const match of request) {
    const result = { i: match.i, j: match.j, stop: count === request.length - 1 };
    let score = 0;
    const left = match.x;
    const right = match.y;
    for (let i = 10; i <= 30; i++) {
      result[i] = bfjoust(left, right, i, 0, false);
      if (result[i].state === "X") score--;
      if (result[i].state === "Y") score++;
    }
    for (let i = 10; i <= 30; i++) {
      result[-i] = bfjoust(left, right, i, 1, false);
      if (result[-i].state === "X") score--;
      if (result[-i].state === "Y") score++;
    }
    result.score = score;
    self.postMessage(result);
    count++;
  }
};
