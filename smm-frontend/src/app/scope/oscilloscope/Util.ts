import { BoundingBoxArray, EmbededOscilloscopeRenderer } from "./Common.interface";
import { Oscilloscope } from "./Oscilloscope";

export const Util = {
    line: function (ctx: any, x0: any, y0: any, x1: any, y1: any, color: any, width: any, dash: any) {
        ctx.beginPath();
        ctx.strokeStyle = color;
        ctx.lineWidth = width;
        ctx.setLineDash(dash);
        ctx.moveTo(x0, y0);
        ctx.lineTo(x1, y1);
        ctx.stroke();
    },
    drawFilledPoly: function (ctx: any, vertices: any, color: any) {
        ctx.beginPath();
        ctx.moveTo(vertices[0].x, vertices[0].y)
        for (let i = 1; i < vertices.length; i++) {
            ctx.lineTo(vertices[i].x, vertices[i].y);
        }
        ctx.closePath();
        ctx.fillStyle = color;
        ctx.fill();
    },
    isPointInBoundingBox(boundingBox: BoundingBoxArray, point: { x: number; y: number; }): boolean {
        const [xMin, yMin, xMax, yMax] = boundingBox;
        return xMin <= point.x && point.x <= xMax && yMin <= point.y && point.y <= yMax;
    },
    translateIntoAndRender(boundingBox: BoundingBoxArray, scope: Oscilloscope, renderer: EmbededOscilloscopeRenderer): void {
        const [xMin, yMin, xMax, yMax] = boundingBox;
        const [newW, newH] = [xMax - xMin, yMax - yMin];
        scope.getRootCtx().translate(xMin, yMin);
        renderer(scope, newW, newH);
        scope.getRootCtx().translate(-xMin, -yMin);
    }
};

export const Formatter = {
    voltageFormatter: (v: number) => { return (v + "").substring(0, 5) + "V" },
    currentFormatter: (v: number) => { return (v + "").substring(0, 5) + "A" },
    powerFormatter: (v: number) => { return (v + "").substring(0, 5) + "W" },
    vaFormatter: (v: number) => { return (v + "").substring(0, 5) + "VA" },
    timeFormatter: (v: number) => { return (v + "").substring(0, 5) + "ms" },
    numFormatter: (v: number) => { return (v + "").substring(0, 5) },
};