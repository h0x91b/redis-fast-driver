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

Redis-fast-driver `node example.js`:

	=================================================
	===
	Start test: PING command 1000 times
	Test complete in 7ms, speed 142857.14 in second, cold down 1.5 sec
	===
	Start test: INCR command 1000 times
	Test complete in 6ms, speed 166666.67 in second, cold down 1.5 sec
	===
	Start test: GET command 1000 times
	Test complete in 6ms, speed 166666.67 in second, cold down 1.5 sec
	===
	Start test: HGET command 1000 times
	Test complete in 7ms, speed 142857.14 in second, cold down 1.5 sec
	===
	Start test: HGETALL command 1000 times
	Test complete in 8ms, speed 125000.00 in second, cold down 1.5 sec
	===
	Start test: ZRANGE 0 4 command 1000 times
	Test complete in 12ms, speed 83333.33 in second, cold down 1.5 sec
	===
	Start test: PING command 5000 times
	Test complete in 15ms, speed 333333.33 in second, cold down 1.5 sec
	===
	Start test: INCR command 5000 times
	Test complete in 16ms, speed 312500.00 in second, cold down 1.5 sec
	===
	Start test: GET command 5000 times
	Test complete in 18ms, speed 277777.78 in second, cold down 1.5 sec
	===
	Start test: HGET command 5000 times
	Test complete in 21ms, speed 238095.24 in second, cold down 1.5 sec
	===
	Start test: HGETALL command 5000 times
	Test complete in 35ms, speed 142857.14 in second, cold down 1.5 sec
	===
	Start test: ZRANGE 0 4 command 5000 times
	Test complete in 32ms, speed 156250.00 in second, cold down 1.5 sec
	===
	Start test: PING command 10000 times
	Test complete in 28ms, speed 357142.86 in second, cold down 1.5 sec
	===
	Start test: INCR command 10000 times
	Test complete in 26ms, speed 384615.38 in second, cold down 1.5 sec
	===
	Start test: GET command 10000 times
	Test complete in 29ms, speed 344827.59 in second, cold down 1.5 sec
	===
	Start test: HGET command 10000 times
	Test complete in 32ms, speed 312500.00 in second, cold down 1.5 sec
	===
	Start test: HGETALL command 10000 times
	Test complete in 62ms, speed 161290.32 in second, cold down 1.5 sec
	===
	Start test: ZRANGE 0 4 command 10000 times
	Test complete in 63ms, speed 158730.16 in second, cold down 1.5 sec
	===
	Start test: PING command 25000 times
	Test complete in 76ms, speed 328947.37 in second, cold down 1.5 sec
	===
	Start test: INCR command 25000 times
	Test complete in 79ms, speed 316455.70 in second, cold down 1.5 sec
	===
	Start test: GET command 25000 times
	Test complete in 97ms, speed 257731.96 in second, cold down 1.5 sec
	===
	Start test: HGET command 25000 times
	Test complete in 99ms, speed 252525.25 in second, cold down 1.5 sec
	===
	Start test: HGETALL command 25000 times
	Test complete in 163ms, speed 153374.23 in second, cold down 1.5 sec
	===
	Start test: ZRANGE 0 4 command 25000 times
	Test complete in 159ms, speed 157232.70 in second, cold down 1.5 sec
	=================================================

Redis benchmark tool with `-q` flag via tcp/ip on same machine:

	PING_INLINE: 125313.29 requests per second
	PING_BULK: 125313.29 requests per second
	SET: 126742.72 requests per second
	GET: 124533.01 requests per second
	INCR: 126742.72 requests per second
	LPUSH: 126422.25 requests per second
	LPOP: 125000.00 requests per second
	SADD: 121951.22 requests per second
	SPOP: 125628.14 requests per second
	LPUSH (needed to benchmark LRANGE): 117785.63 requests per second
	LRANGE_100 (first 100 elements): 31948.88 requests per second
	LRANGE_300 (first 300 elements): 13264.36 requests per second
	LRANGE_500 (first 450 elements): 9039.96 requests per second
	LRANGE_600 (first 600 elements): 6799.95 requests per second
	MSET (10 keys): 87873.46 requests per second

Mocha test (`npm run bench`) of Redis-fast-driver:

	==========================
	redis-fast-driver: 1.0.3
	CPU: 8
	OS: darwin x64
	node version: v4.2.3
	current commit: 0959643
	==========================

	Concurrency 10000
	218,101 op/s » PING
	190,719 op/s » SET foo bar
	183,838 op/s » GET foo
	203,899 op/s » INCR number
	113,289 op/s » HGETALL hset:1
	106,666 op/s » ZRANGE zset:1 0 5
	13,346 op/s » LRANGE list 0 99

	Concurrency 1000
	205,586 op/s » PING
	201,202 op/s » SET foo bar
	209,195 op/s » GET foo
	224,303 op/s » INCR number
	116,750 op/s » HGETALL hset:1
	110,804 op/s » ZRANGE zset:1 0 5
	11,680 op/s » LRANGE list 0 99

	Concurrency 500
	201,103 op/s » PING
	153,441 op/s » SET foo bar
	173,239 op/s » GET foo
	179,958 op/s » INCR number
	108,349 op/s » HGETALL hset:1
	101,903 op/s » ZRANGE zset:1 0 5
	13,840 op/s » LRANGE list 0 99

	Concurrency 250
	195,763 op/s » PING
	148,687 op/s » SET foo bar
	166,859 op/s » GET foo
	169,391 op/s » INCR number
	93,612 op/s » HGETALL hset:1
	85,425 op/s » ZRANGE zset:1 0 5
	13,287 op/s » LRANGE list 0 99

	Concurrency 100
	172,089 op/s » PING
	131,105 op/s » SET foo bar
	147,579 op/s » GET foo
	150,110 op/s » INCR number
	87,084 op/s » HGETALL hset:1
	82,737 op/s » ZRANGE zset:1 0 5
	13,079 op/s » LRANGE list 0 99

	Concurrency 10
	99,971 op/s » PING
	96,470 op/s » SET foo bar
	102,060 op/s » GET foo
	103,722 op/s » INCR number
	53,660 op/s » HGETALL hset:1
	60,193 op/s » ZRANGE zset:1 0 5
	11,081 op/s » LRANGE list 0 99

	Concurrency 1
	23,132 op/s » PING
	19,633 op/s » SET foo bar
	22,256 op/s » GET foo
	21,776 op/s » INCR number
	15,555 op/s » HGETALL hset:1
	19,501 op/s » ZRANGE zset:1 0 5
	7,949 op/s » LRANGE list 0 99

	Suites:  7
	Benches: 49
	Elapsed: 221,065.04 ms

ioredis `npm run bench` on same machine

	==========================
	redis: 2.0.1
	CPU: 8
	OS: darwin x64
	node version: v4.2.3
	current commit: 2ac00c8
	==========================

	SET foo bar
	126,941 op/s » javascript parser + dropBufferSupport: true
	124,539 op/s » javascript parser
	128,321 op/s » hiredis parser + dropBufferSupport: true
	111,211 op/s » hiredis parser

	LRANGE foo 0 99
	21,243 op/s » javascript parser + dropBufferSupport: true
	12,675 op/s » javascript parser
	27,931 op/s » hiredis parser + dropBufferSupport: true
	5,955 op/s » hiredis parser

	Suites:  2
	Benches: 8
	Elapsed: 60,197.37 ms

# Author

Arseniy Pavlenko h0x91b@gmail.com

Skype: h0x91b

Linkedin: https://il.linkedin.com/in/h0x91b

# Licence

(The MIT License)

Copyright (c) 2015-2016 Arseniy Pavlenko h0x91b@gmail.com

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
