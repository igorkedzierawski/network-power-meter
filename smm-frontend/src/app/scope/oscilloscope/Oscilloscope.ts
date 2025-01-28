import { BlockProcessor } from "./BlockProcessor";
import { MouseCoordinates } from "./Common.interface";
import { screenDraw } from "./Renderers";
import { Util } from "./Util";

export class Oscilloscope {

    private readonly rootCtx: CanvasRenderingContext2D;

    private mouseCoords: MouseCoordinates;
    private animationRunning: boolean = false;
    private timeStr: string = '';

    constructor(
        private readonly rootCanvas: HTMLCanvasElement,
        private readonly blockProc: BlockProcessor,
    ) {
        this.rootCtx = rootCanvas.getContext("2d") ??
            (() => { throw new Error("Failed to get 2D context"); })();
        this.mouseCoords = {
            x: 0,
            y: 0,
            clicked: false,
        }

        new ResizeObserver((entries: ResizeObserverEntry[]) => {
            rootCanvas.width = entries[0].contentRect.width;
            rootCanvas.height = entries[0].contentRect.height;
        }).observe(rootCanvas);

        this.registerCanvasEventHandlers();
    }

    private registerCanvasEventHandlers(): void {
        this.rootCanvas.addEventListener('mousemove', (event: MouseEvent) => {
            const rect = this.getRootCanvas().getBoundingClientRect();
            this.mouseCoords.x = event.clientX - rect.left;
            this.mouseCoords.y = event.clientY - rect.top;
        });
        this.rootCanvas.addEventListener('mousedown', (_: any) => {
            this.mouseCoords.clicked = true;
            this.mouseCoords.capturedBy = undefined;
        });
        this.rootCanvas.addEventListener('mouseup', (_: any) => {
            this.mouseCoords.clicked = false;
        });
    }

    public getRootCanvas(): HTMLCanvasElement {
        return this.rootCanvas;
    }

    public getRootCtx(): CanvasRenderingContext2D {
        return this.rootCtx;
    }

    public getTimeStr(): string {
        return this.timeStr;
    }

    public setTimeStr(timeStr: string):void {
        this.timeStr = timeStr;
    }

    public getBlockProcessor(): BlockProcessor {
        return this.blockProc;
    }

    public getRelativeMousePos(): DOMPoint {
        return this.rootCtx.getTransform().inverse().transformPoint(this.mouseCoords);
    }

    public isAnimationRunning(): boolean {
        return this.animationRunning;
    }

    public startAnimation(): void {
        if (this.animationRunning) {
            return;
        }
        this.animationRunning = true;
        this.animationLoopCallback();
    }

    public stopAnimation(): void {
        this.animationRunning = false;
    }

    private animationLoopCallback(): void {
        requestAnimationFrame(() => {
            if (!this.animationRunning) {
                return;
            }
            this.drawFrame();
            this.animationLoopCallback();
        });
    }

    public drawFrame(): void {
        const [W, H] = [this.rootCanvas.width, this.rootCanvas.height];
        this.rootCtx.clearRect(0, 0, W, H);
        Util.translateIntoAndRender([0, 0, W, H], this, screenDraw);
    }

    tryCaptureClick(boundingBox: any, captureName: any) {
        if (!this.mouseCoords.clicked) {
            return false;
        }
        if (this.mouseCoords.capturedBy == captureName) {
            return true;
        }
        if (this.mouseCoords.capturedBy !== undefined) {
            return false;
        }
        if (Util.isPointInBoundingBox(boundingBox, this.getRelativeMousePos())) {
            this.mouseCoords.capturedBy = captureName;
            return true;
        }
        return false;
    }

}
