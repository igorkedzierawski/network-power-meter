import { Component, ViewChild, ElementRef } from '@angular/core';
import { CommonModule } from '@angular/common';
import { Oscilloscope } from './oscilloscope/Oscilloscope';
import { ArraySliderComponent } from './array-slider/array-slider.component';
import { Formatter } from './oscilloscope/Util';
import { RangeMeta } from './oscilloscope/Common.interface';
import { RangeSliderComponent } from './range-slider/range-slider.component';
import { RangeDescriptions } from './misc/RangeDescriptions';
import { BlockProcessor } from './oscilloscope/BlockProcessor';
import { ChannelCheckboxId, MeterCtlComponent } from "./meter-ctl/meter-ctl.component";
import { MainService } from '../main.service';
import { MeterInfo } from '../SMMMock';
import { MeterConnection, WebSocketConnection } from '../WSConnection';
import { RequestEngine, SamplerInfoResponse } from '../request-engine.service';

@Component({
    selector: 'scope-page',
    standalone: true,
    templateUrl: './scope.component.html',
    imports: [
        CommonModule,
        ArraySliderComponent,
        RangeSliderComponent,
        MeterCtlComponent
    ]
})

export class ScopeComponent {

    @ViewChild('canvas', { static: false }) canvasRef!: ElementRef<HTMLCanvasElement>;

    protected readonly RangeDescriptions = RangeDescriptions;

    protected readonly voltageRangeMeta: RangeMeta = {
        name: "Napięcie",
        value: 1,
        unit: "V",
        formatter: Formatter.voltageFormatter,
    };
    protected readonly currentRangeMeta: RangeMeta = {
        name: "Prąd",
        value: 1,
        unit: "A",
        formatter: Formatter.currentFormatter,
    };
    protected readonly powerRangeMeta: RangeMeta = {
        name: "Moc",
        value: 1,
        unit: "W",
        formatter: Formatter.powerFormatter,
    };

    protected readonly baseChannelNames = ["V", "C1", "C2"];

    protected calibrationParams: { [key in string]: number } = {};

    protected readonly blockProc: BlockProcessor = new BlockProcessor([
        { name: this.baseChannelNames[ChannelCheckboxId.Voltage], range: this.voltageRangeMeta.name },
        { name: this.baseChannelNames[ChannelCheckboxId.Current1], range: this.currentRangeMeta.name, color: "green" },
        { name: this.baseChannelNames[ChannelCheckboxId.Current2], range: this.currentRangeMeta.name, color: "purple" },
    ], [this.voltageRangeMeta, this.currentRangeMeta, this.powerRangeMeta]);

    private scope!: Oscilloscope;

    protected voltageRangeChanged(value: number): void {
        this.voltageRangeMeta.value = value;
    }

    protected currentRangeChanged(value: number): void {
        this.currentRangeMeta.value = value;
    }

    protected powerRangeChanged(value: number): void {
        this.powerRangeMeta.value = value;
    }

    protected pageWindowSpan_ms: number = 1000;

    protected windowSpanChanged(value: number): void {
        this.pageWindowSpan_ms = value;
        if (this.meterctl) {
            const newpgSize = this.meterctl.baseFrequency * value / 1000;
            this.blockProc.setPageSize(newpgSize);
        }
        if (this.scope) {
            this.scope.setTimeStr(value + RangeDescriptions.WINDOW_SPAN.unit)
        }
    }

    protected samplerInfo?: SamplerInfoResponse;

    constructor(
        protected readonly reqEng: RequestEngine,
    ) {
    }

    @ViewChild("meterctl") meterctl!: MeterCtlComponent;

    protected connection?: MeterConnection;

    protected mode_ctl: "start" | "stop" = "start";

    async startChangeState(isStart: boolean): Promise<void> {
        if (isStart) {
            const noEnabledChannels = this.meterctl.numOfEnabledChannels;
            if (this.connection || noEnabledChannels === 0) {
                return;
            }
            await this.reqEng.setSamplerSelectedChannels({
                voltage: this.meterctl.checkboxStates[ChannelCheckboxId.Voltage],
                current1: this.meterctl.checkboxStates[ChannelCheckboxId.Current1],
                current2: this.meterctl.checkboxStates[ChannelCheckboxId.Current2],
            });
            await this.reqEng.startSampler();

            this.blockProc.resetPageBuffers();
            this.blockProc.setPageSize(this.meterctl.netFrequency * this.pageWindowSpan_ms / 1000);

            const channelKeys: string[] = this.meterctl.checkboxStates
                .map((state, i) => state ? this.baseChannelNames[i] : undefined)
                .filter(x => x !== undefined) as string[];

            const blockPoolingFrequency_Hz = this.samplerInfo!.freq / this.samplerInfo!.bsamplecapacity;
            const adjustedBlockPoolingDelay_ms = 0.95 * ((1 / blockPoolingFrequency_Hz) * 1000);
            console.log("adjustedBlockPoolingDelay_ms", adjustedBlockPoolingDelay_ms)

            // this.connection = this.mainService.getApi().api_connect();
            this.connection = new WebSocketConnection();
            this.connection.connect(adjustedBlockPoolingDelay_ms, (block) => {
                for (let i = 0; i < 3; i++) {
                    if (block.channels[this.baseChannelNames[i]]) {
                        block.channels[this.baseChannelNames[i]] = block.channels[this.baseChannelNames[i]]!.map(x => {
                            return x === undefined
                                ? undefined
                                : this.calibrationParams[this.baseChannelNames[i] + ".a"] * x + this.calibrationParams[this.baseChannelNames[i] + ".b"];
                        });
                    }
                }
                this.blockProc.submitBlock(block);
            }, channelKeys, this.onDisconnect.bind(this));
            this.scope.startAnimation();
        } else {
            await this.reqEng.stopSampler();
            if (this.connection) {
                this.connection.disconnect();
            }
            if (this.scope) {
                this.scope.stopAnimation();
            }
        }
    }

    onDisconnect() {
        this.mode_ctl = "stop";
        this.connection = undefined;
    }

    ngAfterViewInit() {
        this.scope = new Oscilloscope(this.canvasRef.nativeElement, this.blockProc);

        for (let i = 0; i < 3; i++) {
            this.calibrationParams[this.baseChannelNames[i] + ".a"] = parseFloat(window.localStorage[this.baseChannelNames[i] + ".a"]);
            if(isNaN(this.calibrationParams[this.baseChannelNames[i] + ".a"])) {
                this.calibrationParams[this.baseChannelNames[i] + ".a"] = 1;
            }
            window.localStorage[this.baseChannelNames[i] + ".a"] = this.calibrationParams[this.baseChannelNames[i] + ".a"];

            this.calibrationParams[this.baseChannelNames[i] + ".b"] = parseFloat(window.localStorage[this.baseChannelNames[i] + ".b"]);
            if(isNaN(this.calibrationParams[this.baseChannelNames[i] + ".b"])) {
                this.calibrationParams[this.baseChannelNames[i] + ".b"] = 0;
            }
            window.localStorage[this.baseChannelNames[i] + ".b"] = this.calibrationParams[this.baseChannelNames[i] + ".b"];
        }

        this.reqEng.getSamplerInfo().then(info => {
            this.samplerInfo = info;
        })
        this.reqEng.getSamplerSelectedChannels().then(channels => {
            setTimeout(() => {
                this.meterctl.checkboxStates[ChannelCheckboxId.Voltage] = channels.voltage;
                this.meterctl.checkboxStates[ChannelCheckboxId.Current1] = channels.current1;
                this.meterctl.checkboxStates[ChannelCheckboxId.Current2] = channels.current2;
            }, 200);
        })
        this.reqEng.isSamplerRunning().then(running => {
            this.mode_ctl = running ? "start" : "stop";
        })

        this.blockProc.setTriggerSpec(this.baseChannelNames[ChannelCheckboxId.Voltage], 0.3, true);
    }

}
