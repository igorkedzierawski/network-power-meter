import { SerializedChannels } from "./scope/oscilloscope/Common.interface";
import { MeterConnection } from "./WSConnection";

export class SMMMock {

    protected readonly baseFrequency = 30000;
    protected readonly blockSize = 840;
    protected started: boolean = false;

    protected voltageEnabled: boolean = true;
    protected current1Enabled: boolean = true;
    protected current2Enabled: boolean = true;

    constructor() {
    }

    public async api_getInfo(): Promise<MeterInfo> {
        return new Promise<MeterInfo>((resolve, reject) => {
            setTimeout(() => {
                if (Math.random() > 0.01) {
                    resolve({
                        baseFrequency: this.baseFrequency,
                        meterStarted: this.started,
                        blockSize: this.blockSize,
                    })
                } else {
                    reject("Error wile getting info");
                }
            }, 600 * Math.random())
        });
    }

    public async api_setChannelsEnabled(voltage: boolean, current1: boolean, current2: boolean): Promise<boolean> {
        return new Promise<boolean>((resolve, reject) => {
            setTimeout(() => {
                if (Math.random() > 0.01) {
                    this.voltageEnabled = voltage;
                    this.current1Enabled = current1;
                    this.current2Enabled = current2;
                    resolve(true)
                } else {
                    reject("Error wile getting info");
                }
            }, 600 * Math.random())
        });
    }

    public api_connect(): MeterConnection {
        const blockSize = this.blockSize;
        const noEnabledChannels = (this.voltageEnabled ? 1 : 0) + (this.current1Enabled ? 1 : 0) + (this.current2Enabled ? 1 : 0);
        const effectiveFreq = this.baseFrequency / noEnabledChannels;
        const blocksPerSecond = effectiveFreq / blockSize;
        const millisecondsBetweenBlocks = 1000 / blocksPerSecond;
        let interval: any = null;
        let onCloseCallback = () => { };

        return {
            connect(poolingDelay, blockConsumer, expectedKeys, onClose) {
                if (noEnabledChannels === 0) {
                    onClose();
                    return;
                }
                onCloseCallback = onClose;
                let lastBlockTimestamp = Date.now();
                let n = 0;
                interval = setInterval(() => {
                    if (Date.now() - lastBlockTimestamp < millisecondsBetweenBlocks) {
                        return;
                    }
                    const channelCount = expectedKeys.length;
                    const channels: SerializedChannels = {};
                    let noofundefs = 0;
                    let blockFillage = 0;
                    const channelSettings = [
                        [200, 12],
                        [10, 32],
                        [2, 53],
                    ];
                    while (blockFillage < blockSize) {
                        n++;
                        for (let i = 0; i < channelCount; i++) {
                            if (!channels[expectedKeys[i]]) {
                                channels[expectedKeys[i]] = [];
                            }
                            channels[expectedKeys[i]]![blockFillage] = channelSettings[i][0] * Math.sin(n/23 + channelSettings[i][1]) + Math.random() * channelSettings[i][0]/20;
                        }
                        blockFillage++;
                    }
                    // noofundefs = decodedBlock.buffer.filter(x => x === undefined).length;
                    const block = {
                        index: n,
                        size: blockFillage,
                        channels: channels,
                    }
                    // console.log(avg);
                    // console.log("undefs:", noofundefs);
                    console.log(block);
                    blockConsumer(block);
                    if(Math.random() < 1/effectiveFreq) {
                        clearInterval(interval);
                        this.disconnect();
                    }
                }, poolingDelay);
            },
            disconnect() {
                if (interval == null) {
                    return;
                }
                clearInterval(interval);
                onCloseCallback();
            },
        };
    }

}

export interface MeterInfo {
    baseFrequency: number,
    meterStarted: boolean,
    blockSize: number,
}

export interface SampleStreamHandler {

}