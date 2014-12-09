var redis = require('./build/Release/redis-fast-driver');
var util = require('util');
var EventEmitter = require('events').EventEmitter;

function Redis(opts) {
	var self = this;
	opts.host = opts.host || '127.0.0.1';
	opts.port = opts.port || 6379;
	this.name = opts.name || 'redis-driver['+opts.host+':'+opts.port+']';
	this.ready = false;
	this.readyFirstTime = false;
	this.tryToReconnect = opts.tryToReconnect || true;
	this.queue = [];
	this.redis = new redis.RedisConnector();
	this.reconnectTimeout = opts.reconnectTimeout || 1000;
	this.reconnectTimeoutId = null;
	this.reconnects = 0;
	
	try {
		self.redis.connect(opts.host, opts.port, onConnect, onDisconnect);
	} catch(e) {
		reconnect();
	}
	
	function onConnect(e){
		if(e){
			self.emit('error', e);
			reconnect();
			return;
		}
		self.ready = true;
		if(self.queue.length > 0)
			self.rawCall(['PING'], function(e,d){});
		if(!self.readyFirstTime) {
			self.readyFirstTime = true;
			self.emit('ready');
		}
		self.emit('connected');
	}
	
	function onDisconnect(e){
		if(e){
			self.emit('error', e);
		}
		self.ready = false;
		self.emit('disconnected');
		reconnect();
	}
	
	function reconnect() {
		if(!self.tryToReconnect) return;
		self.reconnects++;
		if(self.reconnectTimeoutId)
			clearTimeout(self.reconnectTimeoutId);
		self.reconnectTimeoutId = setTimeout(function(){
			try {
				self.redis.connect(opts.host, opts.port, onConnect, onDisconnect);
			} catch(e) {
				reconnect();
			}
		}, self.reconnectTimeout);
	}
}

util.inherits(Redis, EventEmitter);

Redis.prototype.rawCall = function(args, cb) {
	var self = this;
	if(!args || !Array.isArray(args)) {
		throw 'first argument must be an Array';
	}
  if(typeof cb === 'undefined') {
    cb = function(e) {
      if(e)
        self.emit('error', e);
    }
  }
	if(!this.ready) {
		this.queue.push({
			args: args,
			cb: cb
		});
		return;
	}
	if(this.queue.length > 0) {
		var queue = this.queue;
		this.queue = [];
		queue.forEach(function(cmd){
			self.redis.redisCmd(cmd.args, cmd.cb);
		});
	}
	this.redis.redisCmd(args, cb);
	return this;
};

module.exports = Redis;