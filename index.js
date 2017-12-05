'use strict';
const redis = require('./build/Release/redis-fast-driver');
const EventEmitter = require('events').EventEmitter;

const defaultOptions = {
  host: '127.0.0.1',
  port: 6379,
  db: 0,
  auth: false,
  maxRetries: 10,
  tryToReconnect: true,
  reconnectTimeout: 1000,
  autoConnect: true,
};

class Redis extends EventEmitter {
  constructor(opts) {
    super();
    this.opts = Object.assign({}, defaultOptions, opts);
    this.init();
  }

  init() {
    const {opts} = this;
    this.name = opts.name || `redis-driver[${opts.host}:${opts.port}]`;
    this.ready = false;
    this.destroyed = false;
    this.readyFirstTime = false;
    this.connecting = false;
    this.queue = [];
    this.redis = new redis.RedisConnector();
    this.reconnectTimeoutId = null;
    this.reconnects = 0;
    this.onDisconnect = this._onDisconnect.bind(this);
    this.onConnect = this._onConnect.bind(this);

    // When autoConnect is on, give the user a chance to bind event handlers
    if (opts.autoConnect) setImmediate(this.connect.bind(this));
  }

  connect() {
    try {
      this.connecting = true;
      this.redis.connect(this.opts.host, this.opts.port, this.onConnect, this.onDisconnect);
    } catch(e) {
      this.reconnect();
    }
  }

  processQueue() {
    if (this.queue.length > 0){
      this.queue.forEach((cmd) => this.redis.redisCmd(cmd.args, cmd.cb));
      this.queue = [];
    }
  }

  reconnect() {
    const {opts} = this;
    if (opts.tryToReconnect === false || this.reconnects >= opts.maxRetries) {
      this.emit('error', new Error('Disconnected, exhausted retries.'));
      this.end();
      return;
    }

    this.reconnects++;
    this.emit('reconnecting', this.reconnects);
    if (this.reconnectTimeoutId) clearTimeout(this.reconnectTimeoutId);
    this.reconnectTimeoutId = setTimeout(this.connect.bind(this), opts.reconnectTimeout);
  }

  selectDb(cb) {
    const dbNum = this.opts.db;
    if (dbNum > 0) {
      this.redis.redisCmd(['SELECT', dbNum], (e) => {
        if (e) {
          this.emit('error', e);
          this.reconnect();
          return;
        }
        cb();
      });
    } else {
      setImmediate(cb);
    }
  }

  sendAuth(cb) {
    const {auth} = this.opts;
    if (auth) {
      this.redis.redisCmd(['AUTH', auth], (e) => {
        if(e) {
          this.emit('error', new Error('Wrong password!'));
          this.reconnect();
          return;
        }
        cb();
      });
    } else {
      setImmediate(cb);
    }
  }

  _onConnect(e) {
    if (e) {
      this.emit('error', e);
      this.reconnect();
      return;
    }
    if (this.destroyed) {
      // end() while we were connecting!
      this.redis && this.redis.disconnect();
      return;
    }

    this.ready = true;
    this.connecting = false;
    this.sendAuth(() => {
      this.selectDb(() => {
        this.processQueue();

        if (!this.readyFirstTime) {
          this.readyFirstTime = true;
          this.emit('ready');
        }
        this.reconnects = 0;
        this.emit('connect');
      });
    });
  }

  _onDisconnect(e) {
    if (this.destroyed) return;
    if (e) {
      this.emit('error', e);
    }
    this.ready = false;
    this.connecting = false;
    this.emit('disconnect');
    this.reconnect();
  }

  rawCall(args, cb) {
    if(!args || !Array.isArray(args)) {
      throw new Error('first argument to rawCall() must be an Array');
    }

    if (typeof cb === 'undefined') {
      cb = (e) => {
        if (e) this.emit('error', e);
      };
    }

    // If not connected, push to queue
    if (!this.ready) {
      this.queue.push({args, cb});
      return this;
    }

    // Send cmd
    this.redis.redisCmd(args, cb);
    return this;
  }

  rawCallAsync(args) {
    return new Promise((resolve, reject) => {
      this.rawCall(args, (err, resp) => {
        if (err) return reject(err);
        resolve(resp);
      });
    });
  }

  end() {
    this.ready = false;
    this.destroyed = true;
    this.queue = null; // prevents possible memleak

    // If we were once connected, disconnect
    if (this.redis && this.readyFirstTime) {
      this.redis.disconnect();
      setImmediate(() => this.emit('disconnect'));
    }
    if (!this.connecting) {
      this.redis = null;
    }
    setImmediate(() => this.emit('end'));
  }
}
module.exports = Redis;
