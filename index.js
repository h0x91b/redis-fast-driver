var redis = require('./build/Release/redis-fast-driver');
var util = require('util');
var EventEmitter = require('events').EventEmitter;
var commandsList = require('./commands-list.js');

function Redis(opts) {
	var self = this;
	opts.host = opts.host || '127.0.0.1';
	opts.port = opts.port || 6379;
	opts.maxretries = opts.maxretries || 10;
	this.name = opts.name || 'redis-driver['+opts.host+':'+opts.port+']';
	this.ready = false;
	this.destroyed = false;
	this.readyFirstTime = false;
	this.tryToReconnect = opts.tryToReconnect || true;
	this.queue = [];
	this.redis = new redis.RedisConnector();
	this.reconnectTimeout = opts.reconnectTimeout || 1000;
	this.reconnectTimeoutId = null;
	this.reconnects = 0;
	
	
	function initialConnect() {
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
			if(self.queue.length > 0){
				var queue = self.queue;
				self.queue = [];
				queue.forEach(function(cmd){
					self.redis.redisCmd(cmd.args, cmd.cb);
				});
			}
			if(!self.readyFirstTime) {
				self.readyFirstTime = true;
				self.emit('ready');
			}
			self.reconnects = 0;
			self.emit('connected');
		}
	
		function onDisconnect(e){
			if(self.destroyed) return;
			if(e){
				self.emit('error', e);
			}
			self.ready = false;
			self.emit('disconnected');
			reconnect();
		}
	
		function reconnect() {
			if(!self.tryToReconnect || self.reconnects > opts.maxretries) return;
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
	process.nextTick(initialConnect);
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

function FakeMulti(instance) {
	var self = this;
	this.instance = instance;
	this.queue = [];
	for(var i=0;i<commandsList.length;i++) {
		if(commandsList[i] === 'exec') continue;
		this[commandsList[i]] = fakeCall(commandsList[i]);
	}
	
	function fakeCall(cmd) {
		return function fakeCall() {
			var args = Array.prototype.slice.call(arguments, 0);
			args.unshift(cmd);
			return self.rawCall(args);
		}
	}
}

FakeMulti.prototype = {
	rawCall: function fakeRawCall(args, cb){
		if(typeof args[args.length - 1] === 'function') {
			cb = args.pop();
		}
		this.queue.push({
			args: args,
			cb: cb
		});
		return this;
	},
	exec: function exec(cb) {
		var self = this;
		var errors = Array(this.queue.length);
		var responses = Array(this.queue.length);
		var remain = this.queue.length;
		
		for(var i=0;i<this.queue.length;i++) {
			this.instance.rawCall(this.queue[i].args, onResponse(i));
		}
		
		function onResponse(index) {
			return function onRedisCallback(e, data) {
				errors[index] = e;
				responses[index] = data;
				if(self.queue[index].cb) {
					try {
						self.queue[index].cb(e, data);
					} catch(e) {
						console.log('exception while running user callback', e);
					}
				}
				if(--remain === 0) {
					if(cb) cb(errors, responses);
				}
			};
		}
		
		return this;
	}
};
//Only for backward compitability with standart redis driver.
Redis.prototype.multi = function(commands) {
	var fake = new FakeMulti(this);
	if(commands) {
		for(var i=0;i<commands.length;i++)
			fake.rawCall(commands[i]);
	}
	return fake;
};

Redis.prototype.end = function() {
	this.ready = false;
	if(this.redis) this.redis.disconnect();
	this.redis = null;
	this.destroyed = true;
};


Redis.print = function(e, resp) {
	console.log('err: %s, response: ', e, resp);
};

module.exports = Redis;
