import { Component, Input, Output, EventEmitter } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';

@Component({
    selector: 'meter-ctl',
    standalone: true,
    templateUrl: './meter-ctl.component.html',
    imports: [
        CommonModule,
        FormsModule,
    ]
})
export class MeterCtlComponent {
    @Input() set mode(value: 'start' | 'stop') {
        this.isStartMode = (value === 'start');
    }
    _baseFrequency = 1000;
    @Input() set baseFrequency(value: number) {
        this._baseFrequency = value;
    }
    get baseFrequency(): number {
        return this._baseFrequency;
    }
    get netFrequency(): number {
        return this._baseFrequency / Math.max(1, this.numOfEnabledChannels);
    }
    get netFrequencyStr(): number {
        return Math.round(this.netFrequency*100)/100;
    }

    @Output() checkboxChanged = new EventEmitter<{ checkboxId: ChannelCheckboxId, checked: boolean }>();
    @Output() startStopChanged = new EventEmitter<boolean>();

    isStartMode = false;
    @Input() checkboxStates: boolean[] = [true, true, true];

    onCheckboxChange(id: number, event: Event) {
        const checked = (event.target as HTMLInputElement).checked;
        this.checkboxStates[id] = checked;
        this.checkboxChanged.emit({ checkboxId: id, checked });
    }

    get allCheckboxesUnchecked(): boolean {
        return this.checkboxStates.every(state => !state);
    }

    get numOfEnabledChannels(): number {
        return this.checkboxStates.filter(state => state).length;
    }

    onToggleStartStop() {
        if (!this.allCheckboxesUnchecked || this.isStartMode) {
            this.isStartMode = !this.isStartMode;
            this.startStopChanged.emit(this.isStartMode);
        }
    }

}

export enum ChannelCheckboxId {
    Voltage = 0,
    Current1 = 1,
    Current2 = 2,
};
