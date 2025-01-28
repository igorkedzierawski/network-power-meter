import { Block, ChannelMeta, TriggerMeta, RangeMeta, ChannelDeclaration, SerializedChannels } from "./Common.interface";

export class BlockProcessor {

    private readonly channels: ReadonlyMap<string, ChannelMeta>;
    private readonly ranges: ReadonlyMap<string, RangeMeta>;

    private pgSize: number;
    private pgTailIndex: number;
    private pgHeadIndex: number;

    private triggerMeta?: TriggerMeta;

    // if appended blocks's size is more than this ratio times the page size,
    // then the appended block will be split into smaller blocks so there
    // is less jitter on the scope when appending big blocks
    private readonly maxBlockSizeToPageSizeRatio: number = 1 / 8;

    constructor(
        channelsDeclarations: ChannelDeclaration[],
        ranges: RangeMeta[],
    ) {
        const rwRanges = new Map<string, RangeMeta>();
        for (const range of ranges) {
            rwRanges.set(range.name, range);
        }
        this.ranges = rwRanges;

        const rwChannels = new Map<string, ChannelMeta>();
        for (const definition of channelsDeclarations) {
            const range = this.ranges.get(definition.range);
            if (!range) {
                throw new Error();
            }
            const spec = {
                name: definition.name,
                range: range,
                scale: 1,
                visible: true,
                color: definition.color || "red",
                data: {
                    rawPage: [],
                    processedPage: [],
                    values: {},
                },
            };
            rwChannels.set(definition.name, spec);
        }
        this.channels = rwChannels;

        this.pgSize = 1;
        this.pgTailIndex = -Infinity;
        this.pgHeadIndex = -Infinity;
    }

    public getChannels(): ReadonlyMap<string, ChannelMeta> {
        return this.channels;
    }

    public getRanges(): ReadonlyMap<string, RangeMeta> {
        return this.ranges;
    }

    public getPageSize(): number {
        return this.pgSize;
    }

    public setPageSize(pageSize: number): void {
        if (pageSize <= 0) {
            throw new Error();
        }
        pageSize = Math.round(pageSize);
        if (this.pgSize > pageSize) {
            for (const [_, channel] of this.channels) {
                channel.data.rawPage = channel.data.rawPage.slice(-pageSize);
                channel.data.processedPage = channel.data.processedPage.slice(-pageSize)
            }
            this.pgHeadIndex = this.pgTailIndex - pageSize + 1;
        }
        this.pgSize = pageSize;
    }

    public getTriggerSpec(): TriggerMeta | undefined {
        return this.triggerMeta;
    }

    public setTriggerSpec(channelName: string, level: number, isRisingEdge: boolean) {
        const meta = this.channels.get(channelName);
        if (!meta) {
            throw new Error();
        }
        this.triggerMeta = { watchedChannel: meta, level, isRisingEdge, };
    }

    public resetPageBuffers() {
        for (const [_, channel] of this.channels) {
            channel.data.rawPage = [];
            channel.data.processedPage = [];
            channel.data.values = {};
        }
        this.pgTailIndex = -Infinity;
        this.pgHeadIndex = -Infinity;
    }

    public submitBlock(block: Block) {
        if (block.size / this.pgSize <= this.maxBlockSizeToPageSizeRatio) {
            this.submitWholeBlock(block);
            return;
        }
        const N = Math.ceil(block.size / this.pgSize / this.maxBlockSizeToPageSizeRatio);
        for (let i = 0; i < N; i++) {
            const from = Math.floor(block.index + (i / N) * block.size);
            const to = Math.floor(block.index + ((i + 1) / N) * block.size);
            const newChannels: SerializedChannels = {};
            for (const channel of this.channels.values()) {
                const channelBlock = block.channels[channel.name];
                if (!channelBlock) {
                    newChannels[channel.name] = "".padEnd(to - from + 1).split("").map(_ => undefined);
                } else {
                    newChannels[channel.name] = block.channels[channel.name]!.slice(from - block.index, to - block.index + 1);
                }
            }
            const newBlock: Block = {
                index: from,
                size: to - from + 1,
                channels: newChannels,
            };
            this.submitWholeBlock(newBlock);
        }
    }

    public submitWholeBlock(block: Block) {
        const blockTailIndex = block.index + block.size - 1;
        if (blockTailIndex <= this.pgTailIndex) {
            return;
        }
        const [makeGap, crosspageJump] = (() => {
            const gap = block.index - this.pgTailIndex - 1;
            if (gap >= this.pgSize) {
                // zresetuj stronę i zacznij ją nowym blokiem
                this.pgHeadIndex = block.index;
                this.pgTailIndex = block.index - 1;
                for (const channel of this.channels.values()) {
                    channel.data.rawPage = [];
                }
                return [false, false];
            }

            const currentBoundaryIndex = this.pgHeadIndex + this.pgSize;
            if (blockTailIndex < currentBoundaryIndex) {
                // dopisz nowy blok do obecnej strony i zrób
                // odstęp między poprzednimi punktami
                return [true, false];
            }

            const triggerIndex = this.findTriggerIndex(blockTailIndex - this.pgTailIndex, this.pgSize);
            if (triggerIndex === undefined) {
                // dopisz nowy blok do obecnej i następnej strony
                // i zrób odstęp między poprzednimi punktami 
                return [true, true];
            } else {
                // przesuń punkty na stronie w miejsce triggera i
                // dopisz nowy blok w stworzone miejsce
                this.pgHeadIndex += triggerIndex;
                for (const channel of this.channels.values()) {
                    channel.data.rawPage = channel.data.rawPage.slice(triggerIndex);
                }
                return [false, false];
            }
        })();

        this.appendPointsFromBlock(block);

        if (crosspageJump) {
            this.pgHeadIndex += this.pgSize;
        }

        if (makeGap) {
            for (const channel of this.channels.values()) {
                for (let i = this.pgTailIndex + 1; i <= this.pgTailIndex + this.pgSize / 50; i++) {
                    channel.data.rawPage[(i - this.pgHeadIndex) % this.pgSize] = undefined;
                }
            }
        }

        this.updateValues();
        this.curateCachedPoints();
    }

    private appendPointsFromBlock(block: Block) {
        const blockTailIndex = block.index + block.size - 1;
        for (const channel of this.channels.values()) {
            const channelBlock = block.channels[channel.name];
            if (!channelBlock) {
                for (let i = this.pgTailIndex + 1; i <= blockTailIndex; i++) {
                    channel.data.rawPage[(i - this.pgHeadIndex) % this.pgSize] = undefined;
                }
            } else {
                for (let i = this.pgTailIndex + 1; i <= blockTailIndex; i++) {
                    channel.data.rawPage[(i - this.pgHeadIndex) % this.pgSize] = i - block.index < 0
                        ? undefined
                        : channelBlock![i - block.index];
                }
            }
        }
        this.pgTailIndex = blockTailIndex;
    }

    private curateCachedPoints() {
        for (const channel of this.channels.values()) {
            const range = channel.range.value;
            channel.data.processedPage = channel.visible
                ? channel.data.rawPage.map((v: any) => {
                    return v === undefined
                        ? undefined
                        : v / range * channel.scale;
                })
                : [];
        }
    }

    private updateValues() {
        for (const channel of this.channels.values()) {
            let undefineds = 0;
            channel.data.values.rms = Math.sqrt(
                channel.data.rawPage
                    .map((x: any) => {
                        if (x !== undefined) {
                            return x * x;
                        }
                        undefineds++;
                        return 0;
                    })
                    .reduce((acc: any, next: any) => { return acc + next }, 0)
                / (channel.data.rawPage.length - undefineds)
            ) * channel.scale;
            channel.data.values.avg = (channel.data.rawPage
                .map((x: any) => x === undefined ? 0 : x)
                .reduce((acc: any, next: any) => { return acc + next }, 0)
                / (channel.data.rawPage.length - undefineds)
            ) * channel.scale;
            channel.data.values.vpp = (channel.data.rawPage
                .reduce((acc: any, next: any) => {
                    if (next < acc[0]) {
                        acc[0] = next;
                    } else if (next > acc[1]) {
                        acc[1] = next;
                    }
                    return acc;
                }, [Infinity, -Infinity]).reduce((acc: any, next: any) => {
                    return acc === undefined ? -next : acc + next;
                }, undefined)
            ) * channel.scale;
        }
    }

    private findTriggerIndex(from: number, to: number) {
        if (!this.triggerMeta) {
            return undefined;
        }
        const channel = this.triggerMeta.watchedChannel;
        const values = channel.data.processedPage;
        const triggerValue = this.triggerMeta.level;;
        const [begin, end] = [
            Math.max(from, 0),
            Math.min(to, this.pgSize - 1)
        ];
        for (let i = begin, prev = undefined; i <= end; i++) {
            if (prev === undefined || values[i] === undefined) {
                prev = values[i];
                continue;
            }
            if (this.triggerMeta.isRisingEdge) {
                if (prev < triggerValue && triggerValue < values[i]!) {
                    return i;
                }
            } else {
                if (prev > triggerValue && triggerValue > values[i]!) {
                    return i;
                }
            }
            prev = values[i];
        }
        return undefined;
    }

}
