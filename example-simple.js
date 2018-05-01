var Redis = require('.');
//var Redis = require('redis-fast-driver');

var r = new Redis({
	//host: '/tmp/redis.sock', //unix domain
	host: '127.0.0.1', //can be IP or hostname
	port: 6379,
	maxretries: 10, //reconnect retries, default -1 (infinity)
	// auth: '123',
	// db: 5
});

//happen only once
r.on('ready', function(){
	console.log('redis ready');
});

//happen each time when reconnected
r.on('connected', function(){
	console.log('redis connected');
});

r.on('disconnected', function(){
	console.log('redis disconnected');
});

r.on('error', function(e){
	console.log('redis error', e);
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