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
r.rawCall(['zadd', 'z', 1, 1, 2, 2, 3, 3, 4, 4], function(err, resp){
	console.log('SET via zadd command returns err: %s, resp: %s', err, resp);
});

r.rawCall(['zpopmin', 'z', '2'], function(e, resp){
	console.log('zpopmin', e, resp);
	r.end();
});