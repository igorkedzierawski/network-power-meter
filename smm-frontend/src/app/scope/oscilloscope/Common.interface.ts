import { Oscilloscope } from "./Oscilloscope";

export interface Block {
    index: number,
    size: number,
    channels: SerializedChannels,
}

export type SerializedChannels = { [key: string]: (number | undefined)[] | undefined };

export interface ChannelDeclaration {
    name: string,
    range: string,
    color?: string,
}

export interface ChannelMeta {
    name: string
    range: RangeMeta,
    scale: number,
    visible: boolean,
    color: string,
    data: {
        rawPage: (number | undefined)[],
        processedPage: (number | undefined)[],
        values: {
            rms?: number,
            avg?: number,
            vpp?: number,
        },
    },
}

export interface TriggerMeta {
    watchedChannel: ChannelMeta,
    level: number
    isRisingEdge: boolean,
}

export interface RangeMeta {
    name: string,
    value: number,
    unit: string,
    formatter: (v: number) => string,
}
export type MouseCoordinates = {
    x: number;
    y: number;
    clicked: boolean;
    capturedBy?: string;
} & DOMPointInit;

export type BoundingBoxArray = [xMin: number, yMin: number, xMax: number, yMax: number];

export type EmbededOscilloscopeRenderer = (scope: Oscilloscope, W: number, H: number) => void;
