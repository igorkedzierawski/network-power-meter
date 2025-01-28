import { Injectable } from "@angular/core";
import { SMMMock } from "./SMMMock";

@Injectable({
    providedIn: 'root'
})
export class MainService {

    private readonly mock: SMMMock;

    constructor(

    ) {
        this.mock = new SMMMock()
    }

    public getApi() {
        return this.mock;
    }

}