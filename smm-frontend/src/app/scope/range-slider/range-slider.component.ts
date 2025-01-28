import { Component, Input, Output, EventEmitter, ViewChild } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';
import { ArraySliderComponent } from '../array-slider/array-slider.component';
import { createFormatterForRangeDescription, RangeDescription } from '../misc/RangeDescriptions';

@Component({
    selector: 'range-slider',
    standalone: true,
    templateUrl: './range-slider.component.html',
    imports: [
        CommonModule,
        FormsModule,
        ArraySliderComponent,
    ]
})
export class RangeSliderComponent {
    
    @ViewChild('slider') slider!: ArraySliderComponent<number>;

    @Input() rangeDescription!: RangeDescription;

    protected selectedValue!: number;
    @Output() onSelectedValueChange = new EventEmitter<number>();

    protected updateSelectedItem(value: any): void {
        this.selectedValue = value;
        this.onSelectedValueChange.emit(this.selectedValue);
    }

    protected createFormatter() {
        return createFormatterForRangeDescription(this.rangeDescription);
    }

}

