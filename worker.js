importScripts("./run.js");
self.onmessage = function (event) {
  dd = event.data;
  cnt = 0;
  for (let d of dd) {
    ppp = { i: d.i, j: d.j, stop: cnt == dd.length - 1 };
    score = 0;
    d.x = preproc(d.x);
    d.y = preproc(d.y);
    for (let i = 10; i <= 30; i++) {
      ppp[i] = bfjoust(d.x, d.y, i, 0);
      if (ppp[i].state == "X") score--;
      if (ppp[i].state == "Y") score++;
    }
    for (let i = 10; i <= 30; i++) {
      ppp[-i] = bfjoust(d.x, d.y, i, 1);
      if (ppp[-i].state == "X") score--;
      if (ppp[-i].state == "Y") score++;
    }
    ppp.score = score;
    self.postMessage(ppp);
    cnt++;
  }
};
