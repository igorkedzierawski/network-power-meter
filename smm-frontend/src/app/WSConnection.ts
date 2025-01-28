import { getOrigin } from "./request-engine.service";
import { Block, SerializedChannels } from "./scope/oscilloscope/Common.interface";

const convert12bitBufferToIntArray = (buffer12bit: Uint8Array) => {
    const output = [];
    for (let i = 0; i < buffer12bit.length; i+=3) {
        let bitwiseReprLeft, bitwiseReprRight;
        bitwiseReprLeft = (buffer12bit[i] << 4) | ((buffer12bit[i + 1] & 0xF0) >> 4);
        bitwiseReprRight = ((buffer12bit[i + 1] & 0x0F) << 8) | buffer12bit[i + 2];
        output.push(bitwiseReprLeft);
        output.push(bitwiseReprRight);
    }
    return output;
};

const convertBytesToBlock = (bytes: Uint8Array) => {
    return {
        first_sample_index: new DataView(bytes.slice(0, 4).reverse().buffer).getUint32(0),
        buffer: convert12bitBufferToIntArray(bytes.slice(4)),
    };
};

/*
#define ENDPOINT_WS_REQUEST_NEXT "NEXT"   /* Fetch next block /
#define ENDPOINT_WS_REQUEST_CLOSE "CLOSE" /* Close connection /

#define ENDPOINT_WS_RESPONSE_NO_BLOCKS "NO_BLOCKS"     /* No blocks available /
#define ENDPOINT_WS_RESPONSE_SAMPLER_OFF "SAMPLER_OFF" /* Sampler is not running /
*/

export class WebSocketConnection implements MeterConnection {

    private socket!: WebSocket
    private interval!: any;

    connect(poolingDelay: number, blockConsumer: (block: Block) => void, expectedKeys: string[], onClose: () => void) {
        if (this.socket) return;
        this.socket = new WebSocket(getOrigin().replace("http", "ws") + "/ws");

        this.socket.binaryType = "arraybuffer";

        this.socket.onopen = () => {
            console.log("WebSocket connection opened");
            this.interval = setInterval(() => {
                this.socket.send("NEXT");
            }, poolingDelay);
        };

        this.socket.onmessage = (event) => {
            if (event.data instanceof ArrayBuffer) {
                const decodedBlock = convertBytesToBlock(new Uint8Array(event.data));
                const blockRealSize = decodedBlock.buffer.length / expectedKeys.length;
                const channelCount = expectedKeys.length;
                const channels: SerializedChannels = {};
                let noofundefs = 0;
                for (let i = 0; i < channelCount; i++) {
                    channels[expectedKeys[i]] = decodedBlock.buffer.filter((_, j) => j % channelCount == i)
                }
                noofundefs = decodedBlock.buffer.filter(x => x === undefined).length;
                const block = {
                    index: decodedBlock.first_sample_index,
                    size: blockRealSize,
                    channels: channels,
                }
                blockConsumer(block);
            } else {
                if (typeof event.data === 'string') {
                    const msg: string = event.data;
                    if (msg === "SAMPLER_OFF") {
                        clearInterval(this.interval);
                        this.disconnect();
                    }
                }
            }
        };

        this.socket.onerror = (error) => {
            console.error("WebSocket error:", error);
            this.disconnect();
        };

        this.socket.onclose = onClose;
    }

    disconnect() {
        if (!this.socket) return;
        try {
            clearInterval(this.interval);
        } catch (e) { }
        try {
            this.socket.close();
        } catch (e) { }
    }

}

export interface MeterConnection {
    connect(
        poolingDelay: number,
        blockConsumer: (block: Block) => void,
        expectedKeys: string[],
        onClose: () => void
    ): void;
    disconnect(): void;
}