import { Injectable } from "@angular/core";
import { HttpClient } from '@angular/common/http';
import { firstValueFrom, map } from "rxjs";

interface IsRunningResponse {
    running: boolean;
}

export interface SamplerInfoResponse {
    freq: number;
    bsamplecapacity: number,
}

interface ChannelsAsNumDTO {
    channels: number;
}

export interface DecodedChannelsInfoDTO {
    voltage: boolean;
    current1: boolean;
    current2: boolean;
}

export function getOrigin(): string {
    return window.location.origin;
}

@Injectable({
    providedIn: 'root'
})
export class RequestEngine {

    private is_mock = false;

    constructor(private readonly http: HttpClient) { }



    async isSamplerRunning(): Promise<boolean> {
        if (this.is_mock) {
            return new Promise((resolve, reject) => {
                resolve(false);
            })
        }
        return firstValueFrom(this.http.get<IsRunningResponse>(getOrigin() + "/sampler/is_running")
            .pipe(map(response => {
                return response.running;
            }))
        );
    }

    async getSamplerInfo(): Promise<SamplerInfoResponse> {
        if (this.is_mock) {
            return new Promise((resolve, reject) => {
                resolve({
                    freq: 30000,
                    bsamplecapacity: 840,
                });
            })
        }
        return firstValueFrom(this.http.get<SamplerInfoResponse>(getOrigin() + "/sampler/get_info"));
    }

    async getSamplerSelectedChannels(): Promise<DecodedChannelsInfoDTO> {
        if (this.is_mock) {
            return new Promise((resolve, reject) => {
                resolve(decodeChannels({ channels: 1 }));
            })
        }
        return firstValueFrom(this.http.get<ChannelsAsNumDTO>(getOrigin() + "/sampler/channels/get_selected")
            .pipe(map(response => {
                return decodeChannels(response);
            }))
        );
    }

    async getSamplerAvailableChannels(): Promise<DecodedChannelsInfoDTO> {
        if (this.is_mock) {
            return new Promise((resolve, reject) => {
                resolve(decodeChannels({ channels: 7 }));
            })
        }
        return firstValueFrom(this.http.get<ChannelsAsNumDTO>(getOrigin() + "/sampler/channels/get_available")
            .pipe(map(response => {
                return decodeChannels(response);
            }))
        );
    }

    async setSamplerSelectedChannels(channels: DecodedChannelsInfoDTO): Promise<void> {
        if (this.is_mock) {
            return new Promise((resolve, reject) => {
                resolve();
            })
        }
        await firstValueFrom(this.http.post(getOrigin() + "/sampler/channels/set_selected", null, {
            params: {
                ...encodeChannels(channels),
            }
        }));
    }

    async startSampler(): Promise<void> {
        if (this.is_mock) {
            return new Promise((resolve, reject) => {
                resolve();
            })
        }
        await firstValueFrom(this.http.post(getOrigin() + "/sampler/start", null));
    }

    async stopSampler(): Promise<void> {
        if (this.is_mock) {
            return new Promise((resolve, reject) => {
                resolve();
            })
        }
        await firstValueFrom(this.http.post(getOrigin() + "/sampler/stop", null));
    }

}

function encodeChannels(decoded: DecodedChannelsInfoDTO): ChannelsAsNumDTO {
    const voltage = decoded.voltage ? 1 : 0;
    const current1 = decoded.current1 ? 1 : 0;
    const current2 = decoded.current2 ? 1 : 0;
    const channels = (current2 << 2) | (current1 << 1) | voltage;
    return { channels };
}

function decodeChannels(encoded: ChannelsAsNumDTO): DecodedChannelsInfoDTO {
    const num = encoded.channels;
    const voltage = (num & 1) !== 0;
    const current1 = (num & 2) !== 0;
    const current2 = (num & 4) !== 0;
    return { voltage, current1, current2 };
}

