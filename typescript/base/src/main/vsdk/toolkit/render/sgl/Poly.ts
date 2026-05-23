import { Numeric } from "../../common/linealAlgebra/Numeric";
import { Polygon } from "./Polygon";
import { PolygonBox } from "./PolygonBox";
import { PolygonClipResult } from "./PolygonClipResult";
import { PolygonClipResultInfo } from "./PolygonClipResultInfo";
import { PolygonVertex } from "./PolygonVertex";
import type { SglContext } from "./SglContext";
import { SglPixelContent } from "./SglPixelContent";
import { Window } from "./Window";

export class Poly {
  private constructor() {
  }

  private static copyVertex(dst: PolygonVertex, src: PolygonVertex): void {
    dst.sx = src.sx;
    dst.sy = src.sy;
    dst.sz = src.sz;
    dst.sw = src.sw;
    dst.x = src.x;
    dst.y = src.y;
    dst.z = src.z;
    dst.u = src.u;
    dst.v = src.v;
    dst.r = src.r;
    dst.g = src.g;
    dst.b = src.b;
  }

  private static polygonSwap(a: Polygon, b: Polygon): void {
    const temporaryN = a.n;
    const temporaryMask = a.mask;
    const temporaryVertices = a.vertices;

    a.n = b.n;
    a.mask = b.mask;
    a.vertices = b.vertices;

    b.n = temporaryN;
    b.mask = temporaryMask;
    b.vertices = temporaryVertices;
  }

  private static setPolygonVertexCoord(vertex: PolygonVertex, index: number, value: number): void {
    switch (index) {
      case 0:
        vertex.sx = value;
        break;
      case 1:
        vertex.sy = value;
        break;
      case 2:
        vertex.sz = value;
        break;
      case 3:
        vertex.sw = value;
        break;
      case 4:
        vertex.x = value;
        break;
      case 5:
        vertex.y = value;
        break;
      case 6:
        vertex.z = value;
        break;
      case 7:
        vertex.u = value;
        break;
      case 8:
        vertex.v = value;
        break;
      case 9:
        vertex.r = value;
        break;
      case 10:
        vertex.g = value;
        break;
      case 11:
        vertex.b = value;
        break;
      default:
        break;
    }
  }

  private static clipToHalfSpace(p: Polygon, q: Polygon, index: number, sign: number, k: number): void {
    q.n = 0;
    q.mask = p.mask;

    let previousVertexIndex = p.n - 1;
    let tu = sign * p.vertices[previousVertexIndex]!.getCoord(index) - p.vertices[previousVertexIndex]!.sw * k;
    for (let currentVertexIndex = 0; currentVertexIndex < p.n; currentVertexIndex++) {
      const u = p.vertices[previousVertexIndex]!;
      const v = p.vertices[currentVertexIndex]!;
      const tv = sign * v.getCoord(index) - v.sw * k;

      if (((tu <= 0.0) && (tv > 0.0)) || ((tu > 0.0) && (tv <= 0.0))) {
        const t = tu / (tu - tv);
        const w = q.vertices[q.n]!;
        for (let attributeIndex = 0, maskBits = p.mask; maskBits !== 0; attributeIndex++, maskBits = maskBits >>> 1) {
          if ((maskBits & 1) !== 0) {
            const uCoord = u.getCoord(attributeIndex);
            const vCoord = v.getCoord(attributeIndex);
            Poly.setPolygonVertexCoord(w, attributeIndex, uCoord + t * (vCoord - uCoord));
          }
        }
        q.n++;
      }

      if (tv <= 0.0) {
        Poly.copyVertex(q.vertices[q.n]!, v);
        q.n++;
      }
      previousVertexIndex = currentVertexIndex;
      tu = tv;
    }
  }

  private static clipAndSwap(elementIndex: number, sign: number, k: number, p: Polygon, q: Polygon, p1: Polygon): void {
    Poly.clipToHalfSpace(p, q, elementIndex, sign, sign * k);
    if (q.n === 0) {
      p1.n = 0;
    }
    Poly.polygonSwap(p, q);
  }

  public static mask(elementOffset: number): number {
    return 1 << (elementOffset / Float64Array.BYTES_PER_ELEMENT);
  }

  public static clipToBox(p1: Polygon, box: PolygonBox): number {
    let x0out = 0;
    let x1out = 0;
    let y0out = 0;
    let y1out = 0;
    let z0out = 0;
    let z1out = 0;
    const p2 = new Polygon();

    if (p1.n + 6 > PolygonClipResultInfo.MAXIMUM_SIDES_PER_POLYGON) {
      process.stderr.write(
        `polyClipToBox: too many vertices: ${p1.n} (max=${PolygonClipResultInfo.MAXIMUM_SIDES_PER_POLYGON}-6)\n`
      );
      process.exit(1);
    }

    for (let i = 0; i < p1.n; i++) {
      const vertex = p1.vertices[i]!;
      if (vertex.sx < box.x0 * vertex.sw) {
        x0out++;
      }
      if (vertex.sx > box.x1 * vertex.sw) {
        x1out++;
      }
      if (vertex.sy < box.y0 * vertex.sw) {
        y0out++;
      }
      if (vertex.sy > box.y1 * vertex.sw) {
        y1out++;
      }
      if (vertex.sz < box.z0 * vertex.sw) {
        z0out++;
      }
      if (vertex.sz > box.z1 * vertex.sw) {
        z1out++;
      }
    }

    if (x0out + x1out + y0out + y1out + z0out + z1out === 0) {
      return PolygonClipResult.POLY_CLIP_IN;
    }

    if (x0out === p1.n || x1out === p1.n || y0out === p1.n ||
      y1out === p1.n || z0out === p1.n || z1out === p1.n) {
      p1.n = 0;
      return PolygonClipResult.POLY_CLIP_OUT;
    }

    let p = p1;
    let q = p2;
    if (x0out !== 0) {
      Poly.clipAndSwap(0, -1.0, box.x0, p, q, p1);
    }
    if (x1out !== 0) {
      Poly.clipAndSwap(0, 1.0, box.x1, p, q, p1);
    }
    if (y0out !== 0) {
      Poly.clipAndSwap(1, -1.0, box.y0, p, q, p1);
    }
    if (y1out !== 0) {
      Poly.clipAndSwap(1, 1.0, box.y1, p, q, p1);
    }
    if (z0out !== 0) {
      Poly.clipAndSwap(2, -1.0, box.z0, p, q, p1);
    }
    if (z1out !== 0) {
      Poly.clipAndSwap(2, 1.0, box.z1, p, q, p1);
    }

    if (p === p2) {
      p1.n = p2.n;
      p1.mask = p2.mask;
      for (let i = 0; i < p2.n; i++) {
        Poly.copyVertex(p1.vertices[i]!, p2.vertices[i]!);
      }
    }
    return PolygonClipResult.POLY_CLIP_PARTIAL;
  }

  private static incrementalizeYFlat(
    p1: PolygonVertex,
    p2: PolygonVertex,
    p: PolygonVertex,
    dp: PolygonVertex,
    y: number
  ): void {
    let dy = p2.sy - p1.sy;
    if (dy === 0.0) {
      dy = 1.0;
    }
    const frac = y + 0.5 - p1.sy;

    dp.sx = (p2.sx - p1.sx) / dy;
    p.sx = p1.sx + dp.sx * frac;
  }

  private static incrementFlat(p: PolygonVertex, dp: PolygonVertex): void {
    p.sx += dp.sx;
  }

  private static scanlineFlat(sglContext: SglContext, y: number, l: PolygonVertex, r: PolygonVertex, win: Window): void {
    let lx = globalThis.Math.ceil(l.sx - 0.5);
    if (lx < win.x0) {
      lx = win.x0;
    }

    let rx = globalThis.Math.floor(r.sx - 0.5);
    if (rx > win.x1) {
      rx = win.x1;
    }
    if (lx > rx) {
      return;
    }

    const rowStart = y * sglContext.width;
    for (let x = lx; x <= rx; x++) {
      const pixelIndex = rowStart + x;
      if (sglContext.pixelData === SglPixelContent.PATCH_POINTER) {
        sglContext.patchBuffer[pixelIndex] = sglContext.currentPatch;
      }
      else if (sglContext.pixelData === SglPixelContent.ELEMENT_POINTER) {
        sglContext.galerkinElementBuffer[pixelIndex] = sglContext.currentGalerkinElement;
      }
      else {
        sglContext.frameBuffer[pixelIndex] = sglContext.currentPixel;
      }
    }
  }

  public static scanFlat(sglContext: SglContext, p: Polygon, win: Window): void {
    let yMin = Numeric.HUGE_DOUBLE_VALUE;
    let top = -1;
    for (let i = 0; i < p.n; i++) {
      if (p.vertices[i]!.sy < yMin) {
        yMin = p.vertices[i]!.sy;
        top = i;
      }
    }

    let li = top;
    let ri = top;
    let rem = p.n;
    let y = globalThis.Math.ceil(yMin - 0.5);
    let ly = y - 1;
    let ry = y - 1;

    const l = new PolygonVertex();
    const r = new PolygonVertex();
    const dl = new PolygonVertex();
    const dr = new PolygonVertex();

    while (rem > 0) {
      while (ly <= y && rem > 0) {
        rem--;
        let i = li - 1;
        if (i < 0) {
          i = p.n - 1;
        }
        Poly.incrementalizeYFlat(p.vertices[li]!, p.vertices[i]!, l, dl, y);
        ly = globalThis.Math.floor(p.vertices[i]!.sy + 0.5);
        li = i;
      }

      while (ry <= y && rem > 0) {
        rem--;
        let i = ri + 1;
        if (i >= p.n) {
          i = 0;
        }
        Poly.incrementalizeYFlat(p.vertices[ri]!, p.vertices[i]!, r, dr, y);
        ry = globalThis.Math.floor(p.vertices[i]!.sy + 0.5);
        ri = i;
      }

      while (y < ly && y < ry) {
        if (y >= win.y0 && y <= win.y1) {
          if (l.sx <= r.sx) {
            Poly.scanlineFlat(sglContext, y, l, r, win);
          }
          else {
            Poly.scanlineFlat(sglContext, y, r, l, win);
          }
        }
        y++;
        Poly.incrementFlat(l, dl);
        Poly.incrementFlat(r, dr);
      }
    }
  }

  private static incrementalizeYZ(
    p1: PolygonVertex,
    p2: PolygonVertex,
    p: PolygonVertex,
    dp: PolygonVertex,
    y: number
  ): void {
    let dy = p2.sy - p1.sy;
    if (dy === 0.0) {
      dy = 1.0;
    }
    const frac = y + 0.5 - p1.sy;

    dp.sx = (p2.sx - p1.sx) / dy;
    p.sx = p1.sx + dp.sx * frac;
    dp.sz = (p2.sz - p1.sz) / dy;
    p.sz = p1.sz + dp.sz * frac;
  }

  private static incrementZ(p: PolygonVertex, dp: PolygonVertex): void {
    p.sx += dp.sx;
    p.sz += dp.sz;
  }

  private static longCast(value: number): number {
    if (!Number.isFinite(value)) {
      return value > 0 ? Number.MAX_SAFE_INTEGER : Number.MIN_SAFE_INTEGER;
    }
    return globalThis.Math.trunc(value);
  }

  private static scanlineZ(sglContext: SglContext, y: number, l: PolygonVertex, r: PolygonVertex, win: Window): void {
    let lx = globalThis.Math.ceil(l.sx - 0.5);
    if (lx < win.x0) {
      lx = win.x0;
    }

    let rx = globalThis.Math.floor(r.sx - 0.5);
    if (rx > win.x1) {
      rx = win.x1;
    }
    if (lx > rx) {
      return;
    }

    let dx = r.sx - l.sx;
    if (dx === 0.0) {
      dx = 1.0;
    }

    const frac = lx + 0.5 - l.sx;
    const dzf = (r.sz - l.sz) / dx;
    let z = Poly.longCast(l.sz + dzf * frac);
    const dz = Poly.longCast(dzf);

    const rowStart = y * sglContext.width;
    let pixelIndex = rowStart + lx;
    for (let x = lx; x <= rx; x++) {
      if (z <= (sglContext.depthBuffer as number[])[pixelIndex]!) {
        if (sglContext.pixelData === SglPixelContent.PATCH_POINTER) {
          sglContext.patchBuffer[pixelIndex] = sglContext.currentPatch;
        }
        else if (sglContext.pixelData === SglPixelContent.ELEMENT_POINTER) {
          sglContext.galerkinElementBuffer[pixelIndex] = sglContext.currentGalerkinElement;
        }
        else {
          sglContext.frameBuffer[pixelIndex] = sglContext.currentPixel;
        }
        (sglContext.depthBuffer as number[])[pixelIndex] = z;
      }
      pixelIndex++;
      z += dz;
    }
  }

  public static scanZ(sglContext: SglContext, p: Polygon, window: Window): void {
    let yMin = Numeric.HUGE_DOUBLE_VALUE;
    let top = -1;
    for (let i = 0; i < p.n; i++) {
      if (p.vertices[i]!.sy < yMin) {
        yMin = p.vertices[i]!.sy;
        top = i;
      }
    }

    let li = top;
    let ri = top;
    let rem = p.n;
    let y = globalThis.Math.ceil(yMin - 0.5);
    let ly = y - 1;
    let ry = y - 1;

    const l = new PolygonVertex();
    const r = new PolygonVertex();
    const dl = new PolygonVertex();
    const dr = new PolygonVertex();

    while (rem > 0) {
      while (ly <= y && rem > 0) {
        rem--;
        let i = li - 1;
        if (i < 0) {
          i = p.n - 1;
        }
        Poly.incrementalizeYZ(p.vertices[li]!, p.vertices[i]!, l, dl, y);
        ly = globalThis.Math.floor(p.vertices[i]!.sy + 0.5);
        li = i;
      }

      while (ry <= y && rem > 0) {
        rem--;
        let i = ri + 1;
        if (i >= p.n) {
          i = 0;
        }
        Poly.incrementalizeYZ(p.vertices[ri]!, p.vertices[i]!, r, dr, y);
        ry = globalThis.Math.floor(p.vertices[i]!.sy + 0.5);
        ri = i;
      }

      while (y < ly && y < ry) {
        if (y >= window.y0 && y <= window.y1) {
          if (l.sx <= r.sx) {
            Poly.scanlineZ(sglContext, y, l, r, window);
          }
          else {
            Poly.scanlineZ(sglContext, y, r, l, window);
          }
        }
        y++;
        Poly.incrementZ(l, dl);
        Poly.incrementZ(r, dr);
      }
    }
  }
}
