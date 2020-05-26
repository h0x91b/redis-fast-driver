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
	r.end(); // Close connection
});