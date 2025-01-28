import { BoundingBoxArray } from "./Common.interface";
import { Oscilloscope } from "./Oscilloscope";
import { SCOPE_STYLE } from "./ScopeStyle";
import { Util } from "./Util";

export function screenDraw(scope: Oscilloscope, W: number, H: number) {
    let gapLeft = 0;
    if (scope.getBlockProcessor().getTriggerSpec()) {
        Util.translateIntoAndRender([
            0, 0, SCOPE_STYLE.TRIGGER.WIDTH, H - SCOPE_STYLE.INFO.HEIGHT
        ], scope, drawTriggerWidget);
        gapLeft = SCOPE_STYLE.TRIGGER.WIDTH;
    }
    Util.translateIntoAndRender([
        gapLeft, 0, W, H - SCOPE_STYLE.INFO.HEIGHT
    ], scope, (scope, W, H) => {
        drawBackground(scope, W, H);
        drawPlots(scope, W, H);
    });
    Util.translateIntoAndRender([
        0, H - SCOPE_STYLE.INFO.HEIGHT, W, H
    ], scope, drawInfo);
}

export function drawInfo(scope: Oscilloscope, W: number, H: number) {
    const ctx = scope.getRootCtx();
    const style = SCOPE_STYLE.INFO;
    ctx.fillStyle = style.BG_COLOR;
    ctx.fillRect(0, 0, W, H);

    const paddingX = style.PADDING_X;
    const paddedBox: BoundingBoxArray = [paddingX, 0, W - paddingX, H];

    Util.translateIntoAndRender(paddedBox, scope, (scope, W, H) => {
        const ctx = scope.getRootCtx();
        const initialTransform = ctx.getTransform();
        const yRealMid = H / 2;
        const fontSize = style.FONT_SIZE;
        const yTextMid = yRealMid + fontSize / 2;

        ctx.font = style.FONT_SIZE + "px " + style.FONT_NAME;
        ctx.fillStyle = style.FONT_COLOR;

        const triggerSpec = scope.getBlockProcessor().getTriggerSpec();
        if (triggerSpec) {
            const initialY = fontSize / 2 * (triggerSpec.isRisingEdge ? 1 : -1);
            ctx.strokeStyle = style.TRIGGER.COLOR;
            ctx.lineWidth = fontSize / 5;
            ctx.setLineDash([]);
            ctx.beginPath();
            ctx.moveTo(0, initialY + yRealMid);
            ctx.lineTo(fontSize / 2, initialY + yRealMid);
            ctx.lineTo(fontSize / 2, -initialY + yRealMid);
            ctx.lineTo(fontSize, -initialY + yRealMid);
            ctx.stroke()
            ctx.translate(fontSize + style.GAP_X, 0);

            const channel = triggerSpec.watchedChannel;
            const scale = channel.scale;
            const range = channel.range;
            const triggerValue = triggerSpec.level / scale * range.value;
            const triggerValueStr = (
                triggerSpec.watchedChannel.name.substring(0, style.TRIGGER.CHANNEL_NAME_LENGTH) + " "
                + range.formatter(triggerValue).substring(0, style.TRIGGER.VALUE_LENGTH)
            ).padEnd(style.TRIGGER.CHANNEL_NAME_LENGTH + style.TRIGGER.VALUE_LENGTH + 4);
            ctx.fillText(triggerValueStr, 0, yTextMid)
            ctx.translate(triggerValueStr.length * fontSize / 2 + style.GAP_X, 0);
        }

        const timeInfo = ("Time:" + scope.getTimeStr()).padEnd(style.TIME_LABEL_LENGTH + 5);
        // const timeInfo = ("Time:" + this.blockProc.getTimeRange() + "ms").padEnd(style.TIME_LABEL_LENGTH + 5);
        ctx.fillText(timeInfo, 0, yTextMid)
        ctx.translate(timeInfo.length * fontSize / 2 + style.GAP_X, 0);

        ctx.setTransform(initialTransform);
    });
}

export function drawPlots(scope: Oscilloscope, W: number, H: number) {
    const ctx = scope.getRootCtx();
    const mousePos = scope.getRelativeMousePos();
    const pageSize = scope.getBlockProcessor().getPageSize();
    const channelSpecs = scope.getBlockProcessor().getChannels();
    for (const meta of channelSpecs.values()) {
        const points = meta.data.processedPage;
        ctx.strokeStyle = meta.color;
        ctx.lineWidth = SCOPE_STYLE.PLOT.LINE_WIDTH;
        ctx.setLineDash([]);
        let prevY = undefined;
        for (let i = 0; i < pageSize; i++) {
            const [currX, currY] = [W * i / (pageSize - 1), points[i]];
            if (currY === undefined) {
                prevY !== undefined && ctx.stroke();
            } else if (prevY !== undefined) {
                ctx.lineTo(currX, ((-currY + 1) / 2) * H);
            } else {
                ctx.beginPath();
                ctx.moveTo(currX, ((-currY + 1) / 2) * H);
            }
            prevY = currY;
        }
        prevY !== undefined && ctx.stroke();
    }
    if (Util.isPointInBoundingBox([0, 0, W, H], scope.getRelativeMousePos())) {
        Util.line(ctx, mousePos.x, 0, mousePos.x, H, "#000", 1, []);
        Util.line(ctx, 0, mousePos.y, W, mousePos.y, "#000", 1, []);
    }
}

export function drawBackground(scope: Oscilloscope, W: number, H: number) {
    const ctx = scope.getRootCtx();
    const [vSteps, hSteps] = [10, 10];
    const [vDiv, hDiv] = [H / vSteps, W / hSteps];
    const style = SCOPE_STYLE.BACKGROUND;
    ctx.fillStyle = style.BG_COLOR;
    ctx.fillRect(0, 0, W, H);
    for (let i = 1; i < vSteps; i++) {
        const y = i * vDiv;
        Util.line(ctx, 0, y, W, y, style.LINE_COLOR, style.LINE_WIDTH, style.LINE_DASH);
    }
    for (let i = 1; i < hSteps; i++) {
        const x = i * hDiv;
        Util.line(ctx, x, 0, x, H, style.LINE_COLOR, style.LINE_WIDTH, style.LINE_DASH);
    }
}

export function drawTriggerWidget(scope: Oscilloscope, W: number, H: number) {
    const ctx = scope.getRootCtx();
    const style = SCOPE_STYLE.TRIGGER;
    ctx.fillStyle = style.BG_COLOR;
    ctx.fillRect(0, 0, W, H);
    const triggerSpec = scope.getBlockProcessor().getTriggerSpec()!;
    const paddingX = style.PADDING_X;
    const paddedBox: BoundingBoxArray = [paddingX, 0, W - paddingX, H];
    Util.translateIntoAndRender(paddedBox, scope, (scope, W, H) => {
        if (scope.tryCaptureClick(paddedBox, "trigger")) {
            let y = scope.getRelativeMousePos().y;
            y = Math.max(Math.min(H, y), 0);
            triggerSpec.level = -2 * y / H + 1;
        }
        const triggerPosition = H * (-triggerSpec.level + 1) / 2;
        const height = style.KNOB_HEIGHT;
        Util.drawFilledPoly(ctx, [
            { x: 0, y: triggerPosition - height / 2 },
            { x: W, y: triggerPosition },
            { x: 0, y: triggerPosition + height / 2 },
        ], style.KNOB_COLOR);
    });
}