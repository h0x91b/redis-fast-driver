# Redis-fast-driver

Trully async redis driver designed for max performance. Extremly simple, extremely fast.

This node module uses hiredis async library for connection and for parsing written on C by Salvatore Sanfilippo.

All redis commands including PUB/SUB and MONITOR works fine, this driver 2 years on my production enviroment under really high load (30k ops/sec each redis in cluster).
Also this driver used in https://github.com/joaojeronimo/node_redis_cluster and in my fork https://github.com/h0x91b/fast-redis-cluster

[![Build Status](https://travis-ci.org/h0x91b/redis-fast-driver.svg?branch=master)](https://travis-ci.org/h0x91b/redis-fast-driver)

# Installing
```
npm install redis-fast-driver --save
```

# Usage

Check `example*.js` for usage.

```js
var Redis = require('redis-fast-driver');

var r = new Redis({
	//host: '/tmp/redis.sock', //unix domain
	host: '127.0.0.1', //can be IP or hostname
	port: 6379,
	maxRetries: 10, //reconnect retries, default -1 (infinity)
	auth: '123', //optional password, if needed
	db: 5, //optional db selection
	autoConnect: true, //will connect after creation
	doNotSetClientName: false, //will set connection name (you can see connections by running CLIENT LIST on redis server)
	doNotRunQuitOnEnd: false, //when you call `end()`, driver tries to send `QUIT` command to redis before actual end
});

//happen only once
r.on('ready', function(){
	console.log('redis ready');
});

//happen each time when reconnected
r.on('connect', function(){
	console.log('redis connected');
});

r.on('disconnect', function(){
	console.log('redis disconnected');
});

r.on('reconnecting', function(num){
	console.log('redis reconnecting with attempt #' + num);
});

r.on('error', function(e){
	console.log('redis error', e);
});

// called on an explicit end, or exhausted reconnections
r.on('end', function() {
	console.log('redis closed');
});

//rawCall function has 2 arguments,
//1 - array which contain a redis command
//2 - optional callback
//Redis command is case insesitive, e.g. you can specify HMGET as HMGET, hmget or HmGeT
//but keys and value are case sensitive, foo, Foo, FoO not the same...
r.rawCall(['set', 'foo', 'bar'], function(err, resp){
	console.log('SET via rawCall command returns err: %s, resp: %s', err, resp);
});

r.rawCall(['ping'], function(e, resp){
	console.log('ping', e, resp);
});

// Also has built-in ES6 Promise support
r.rawCallAsync(['ping'])
.then((resp) => {
	console.log('ping', resp);
})
.catch((e) => {
	console.error(e);
});

//types are decoded exactly as redis returns it
//e.g. GET will return string
r.rawCall(['set', 'number', 123]);
r.rawCall(['get', 'number'], function(err, resp){
	//type of "resp" will be "string"
	//this is not related to driver this is behaviour of redis...
	console.log('The value: "%s", number key becomes typeof %s', resp, typeof resp);
});

//but INCR command on same key will return a number
r.rawCall(['incr', 'number'], function(err, resp){
	//type of "resp" will be a "number"
	console.log('The value after INCR: "%s", number key becomes typeof %s', resp, typeof resp);
});
//"number" type will be also on INCRBY ZSCORE HLEN and each other redis command which return a number.

//ZRANGE will return an Array, same as redis returns..
r.rawCall(['zadd', 'sortedset', 1, 'a', 2, 'b', 3, 'c']);
r.rawCall(['zrange', 'sortedset', 0, -1], function(err, resp){
	//type of will be "number"
	console.log('JSON encoded value of zrange: %s', JSON.stringify(resp));
});

//SCAN, HSCAN, SSCAN and other *SCAN* commands will return an Array within Array, like this:
// [ 245, ['key1', 'key2', 'key3'] ]
// first entry (245) - cursor, second one - Array of keys.
r.rawCall(['hscan', 'hset:1', 0], function(e, resp){
	console.log('hscan 0', e, resp);
});

r.rawCall(['hmset', 'hset:1', 'a', 1, 'b', 2, 'c', 3], function(e, resp){
	console.log('hmset', e, resp);
});

r.rawCall(['zadd', 'zset:1', 1, 'a', 2, 'b', 3, 'c', 4, 'd'], function(e, resp){
	console.log('zset', e, resp);
});

//HMGET and HGETALL also returns an Array
r.rawCall(['hgetall', 'hset:1'], function(e, resp){
	console.log('HGETALL', e, resp);
});

r.rawCall(['zrange', 'zset:1', 0, -1], function(e, resp){
	console.log('ZRANGE', e, resp);
	//disconnect
	r.end();
});
````

# Speed

Works MUCH faster then node-redis, 20-50% faster then `ioredis` and even faster then `redis-benchmark` tool.

Results for my MacBook Pro (Retina, 15-inch, Mid 2014, 2.5 GHz Intel Core i7) via tcp/ip.

Redis benchmark tool with `-q` flag via tcp/ip on this machine:

	PING_INLINE: 85324.23 requests per second
	PING_BULK: 85034.02 requests per second
	SET: 85034.02 requests per second
	GET: 84317.03 requests per second
	INCR: 87032.20 requests per second
	LPUSH: 85397.09 requests per second
	RPUSH: 86505.19 requests per second
	LPOP: 85106.38 requests per second
	RPOP: 84961.77 requests per second
	SADD: 86132.64 requests per second
	HSET: 86058.52 requests per second
	SPOP: 86430.43 requests per second
	LPUSH (needed to benchmark LRANGE): 84817.64 requests per second
	LRANGE_100 (first 100 elements): 26441.04 requests per second
	LRANGE_300 (first 300 elements): 11273.96 requests per second
	LRANGE_500 (first 450 elements): 7881.46 requests per second
	LRANGE_600 (first 600 elements): 6071.28 requests per second
	MSET (10 keys): 78064.01 requests per second

Mocha test (`npm run bench` you will need to install `npm i matcha` before, excluded it because of security alerts) of Redis-fast-driver:

	==========================
	redis-fast-driver: 2.1.2
	CPU: 8
	OS: darwin x64
	node version: v8.9.4
	current commit: 7694b0d
	==========================
	
	Concurrency 10000
	475,043 op/s » PING
	358,870 op/s » SET foo bar
	406,564 op/s » GET foo
	437,724 op/s » INCR number
	185,442 op/s » HGETALL hset:1
	181,277 op/s » ZRANGE zset:1 0 5
	17,493 op/s » LRANGE list 0 99
	
	Concurrency 1000
	368,027 op/s » PING
	294,628 op/s » SET foo bar
	314,100 op/s » GET foo
	345,316 op/s » INCR number
	155,711 op/s » HGETALL hset:1
	158,416 op/s » ZRANGE zset:1 0 5
	16,769 op/s » LRANGE list 0 99
	
	Concurrency 500
	364,537 op/s » PING
	254,194 op/s » SET foo bar
	297,171 op/s » GET foo
	310,571 op/s » INCR number
	142,677 op/s » HGETALL hset:1
	143,808 op/s » ZRANGE zset:1 0 5
	16,230 op/s » LRANGE list 0 99
	
	Concurrency 250
	343,095 op/s » PING
	242,263 op/s » SET foo bar
	280,271 op/s » GET foo
	293,171 op/s » INCR number
	141,413 op/s » HGETALL hset:1
	135,673 op/s » ZRANGE zset:1 0 5
	15,667 op/s » LRANGE list 0 99
	
	Concurrency 100
	293,601 op/s » PING
	213,687 op/s » SET foo bar
	245,684 op/s » GET foo
	254,148 op/s » INCR number
	126,322 op/s » HGETALL hset:1
	121,123 op/s » ZRANGE zset:1 0 5
	15,474 op/s » LRANGE list 0 99
	
	Concurrency 10
	157,582 op/s » PING
	132,260 op/s » SET foo bar
	143,180 op/s » GET foo
	145,747 op/s » INCR number
	92,034 op/s » HGETALL hset:1
	89,295 op/s » ZRANGE zset:1 0 5
	14,229 op/s » LRANGE list 0 99
	
	Concurrency 1
	26,111 op/s » PING
	24,870 op/s » SET foo bar
	25,364 op/s » GET foo
	25,348 op/s » INCR number
	22,255 op/s » HGETALL hset:1
	21,989 op/s » ZRANGE zset:1 0 5
	10,139 op/s » LRANGE list 0 99

ioredis `npm run bench` on same machine for comparison

	==========================
	redis: 3.2.2
	CPU: 8
	OS: darwin x64
	node version: v8.9.4
	current commit: 988d8d9
	==========================
	
	SET foo bar
	87,895 op/s » javascript parser + dropBufferSupport: true
	90,263 op/s » javascript parser
	
	LRANGE foo 0 99
	37,174 op/s » javascript parser + dropBufferSupport: true
	24,955 op/s » javascript parser

# Pipelining

The driver is working using async API, async API works by nature as pipelining, so no speed boost will be archived.

But any way you still can use pipelining to make sure that all transaction will be done in one request:

Example:

```js
// init pipeline mode
r.rawCall(['multi']);
	r.rawCall(['echo', 'hello world']);
	r.rawCall(['ping']);
	r.rawCall(['incr', 'qqq']);
	r.rawCall(['set', 'foo', 'bar']);
	r.rawCall(['get', 'foo']);
// execute all commands above once
r.rawCall(['exec'], function(e, resp){
	console.log('exec resp', resp);
	// exec resp [ 'hello world', 'PONG', 1, 'OK', 'bar' ]
});
```

# Author

Arseniy Pavlenko h0x91b@gmail.com

Linkedin: https://il.linkedin.com/in/h0x91b

# Contributors

* Samuel Reed (https://github.com/strml)
* Adrian Gierakowski (https://github.com/adrian-gierakowski)
* HcgRandon (https://github.com/HcgRandon)

# Licence

(The MIT License)

Copyright (c) 2015-2018 Arseniy Pavlenko h0x91b@gmail.com

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
