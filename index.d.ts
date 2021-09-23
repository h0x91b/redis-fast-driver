/// <reference types="node" />
import { EventEmitter } from 'events';

interface RedisOptions {
    host: string;
    port: number;
    db: number;
    auth: false | string;
    maxRetries: number;
    tryToReconnect: boolean;
    reconnectTimeout: number;
    autoConnect: boolean;
    doNotSetClientName: boolean;
    doNotRunQuitOnEnd: boolean;
}

type Callback<T = string> = (err: Error | null, result: T) => void;

declare class Redis extends EventEmitter {
    constructor(opts?: RedisOptions);
    init(): void;
    connect(): void;
    processQueue(): void;
    reconnect(): void;
    selectDb(cb: Callback): void;
    sendAuth(cb: Callback): void;
    rawCall<T = string>(args: (string | Buffer)[], cb?: Callback<T>): void;
    rawCallAsync<T = string>(args: (string | Buffer)[]): Promise<T>;
    end(): void;
}
