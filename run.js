function expand(x) {
  stack = [];
  id = 0;
  res = "";
  for (let i of x) {
    if (i == "(") {
      stack.push(id);
    } else if (i == ")") {
      k = stack.pop();
      if (!stack.length) {
        if (x[id + 1] == "*") {
          w = parseInt(x.slice(id + 2));
          r = "";
          for (let j = 0; j < w; j++) r += x.slice(k + 1, id);
          res += r;
        } else {
          w = parseInt(x.slice(id + 2));
          z = x.slice(k + 1, id);
          r = "";
          for (let j = 0; j < w; j++) r += z.slice(0, z.indexOf("{"));
          r += z.slice(z.indexOf("{") + 1, z.lastIndexOf("}"));
          for (let j = 0; j < w; j++) r += z.slice(z.lastIndexOf("}") + 1);
          res += r;
        }
      }
    } else {
      if (!stack.length) {
        res += i;
      }
    }
    id++;
  }
  return res;
}
function preproc(bf) {
  c = 0;
  while (expand(bf) != bf) {
    bf = expand(bf);
    c++;
  }
  r = "";
  for (let i of bf) {
    if ("+-.<>[]".includes(i)) {
      r += i;
    }
  }
  return r;
}
function bfjoust(x, y, tapesize, switched) {
  tape = [];
  for (let i = 0; i < tapesize; i++) tape.push(0);
  stack = [];
  mx = {};
  my = {};
  for (var i = 0; i < x.length; i++) {
    if (x[i] == "[") {
      stack.push(i);
    }
    if (x[i] == "]") {
      if (stack.length == 0) {
        throw new Error("Right bracket does not match left bracket");
      }
      var mt = stack.pop();
      mx[mt] = i;
      mx[i] = mt;
    }
  }
  if (stack.length != 0) {
    throw new Error("Left bracket does not match right bracket");
  }
  for (var i = 0; i < y.length; i++) {
    if (y[i] == "[") {
      stack.push(i);
    }
    if (y[i] == "]") {
      if (stack.length == 0) {
        throw new Error("Right bracket does not match left bracket");
      }
      var mt = stack.pop();
      my[mt] = i;
      my[i] = mt;
    }
  }
  if (stack.length != 0) {
    throw new Error("Left bracket does not match right bracket");
  }
  px = 0;
  py = tapesize - 1;
  ipx = 0;
  ipy = 0;
  tape[px] = tape[py] = 128;
  fx = 0;
  fy = 0;
  lx = 0;
  ly = 0;
  rx = "";
  ry = "";
  for (let i = 0; i < 100000; i++) {
    curx = tape[px];
    cury = tape[py];
    switch (x[ipx]) {
      case "+": {
        tape[px]++;
        tape[px] &= 255;
        break;
      }
      case "-": {
        tape[px]--;
        tape[px] &= 255;
        break;
      }
      case "<": {
        if (!px) {
          lx = 1;
          rx = "pointer off own end";
        }
        px--;
        break;
      }
      case ">": {
        if (px == tapesize - 1) {
          lx = 1;
          rx = "pointer off opponent end";
        }
        px++;
        break;
      }
      case "[": {
        if (!curx) ipx = mx[ipx];
        break;
      }
      case "]": {
        if (curx) ipx = mx[ipx];
        break;
      }
    }
    ipx++;
    switch (y[ipy]) {
      case "+": {
        if (!switched) tape[py]++;
        else tape[py]--;
        tape[py] &= 255;
        break;
      }
      case "-": {
        if (!switched) tape[py]--;
        else tape[py]++;
        tape[py] &= 255;
        break;
      }
      case ">": {
        if (!py) {
          ly = 1;
          ry = "pointer off opponent end";
        }
        py--;
        break;
      }
      case "<": {
        if (py == tapesize - 1) {
          ly = 1;
          ry = "pointer off own end";
        }
        py++;
        break;
      }
      case "[": {
        if (!cury) ipy = my[ipy];
        break;
      }
      case "]": {
        if (cury) ipy = my[ipy];
        break;
      }
    }
    ipy++;
    if (!tape[0]) {
      if (fx) {
        lx = 1;
        rx = "flag fell";
      } else fx = 1;
    } else fx = 0;
    if (!tape[tapesize - 1]) {
      if (fy) {
        ly = 1;
        ry = "flag fell";
      } else fy = 1;
    } else fy = 0;
    if (lx && ly) {
      return { state: "T", reason: `left: ${rx}\tright: ${ry}` };
    } else if (lx) {
      return { state: "X", reason: rx };
    } else if (ly) {
      return { state: "Y", reason: ry };
    }
  }
  return { state: "T", reason: "timeout" };
}
