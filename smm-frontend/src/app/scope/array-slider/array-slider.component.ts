import { Component, Input, Output, EventEmitter, SimpleChange } from '@angular/core';
import { CommonModule } from '@angular/common';
import { FormsModule } from '@angular/forms';

@Component({
    selector: 'array-slider',
    standalone: true,
    templateUrl: './array-slider.component.html',
    imports: [
        CommonModule,
        FormsModule,
    ]
})
export class ArraySliderComponent<T> {

    @Input() items: T[] = [];
    @Input() title: string = '';
    @Input() formatter?: ((v: T) => string);
    @Input() selectedIndex: number = 0;
    
    protected selectedItem: T | null = null;
    @Output() onSelectedItemChange = new EventEmitter<T | null>();

    ngOnInit() {
        this.updateSelectedItem();
    }

    protected formatItem(item: T | null) {
        if (this.formatter && item) {
            return this.formatter(item);
        }
        return item + "";
    }

    protected updateSelectedItem(): void {
        if (this.items.length === 0) {
            this.selectedItem = null;
        } else {
            this.selectedItem = this.items[this.selectedIndex];
        }
        this.onSelectedItemChange.emit(this.selectedItem);
    }

}

