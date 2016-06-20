var Redis = require('.');
//var Redis = require('redis-fast-driver');

var r = new Redis({
	host: '127.0.0.1',
	port: 6379
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
r.rawCall(['hmset', 'hset:1', 'a', 1, 'b', 2, 'c', 3]);
r.rawCall(['zadd', 'zset:1', 1, 'a', 2, 'b', 3, 'c', 4, 'd']);

var pings = 3;

function ping() {
	r.rawCall(['PING'], function(e, d){
		console.log('PING', e, d);
	});
	
	r.rawCall(['hmget', 'hset:1', 'a', 'b', 'c', 'f'], function(e,d,size){
		console.log('hmget', e, d, size);
	});
	
	r.rawCall(['zrange', 'zset:1', 0, -1, 'WITHSCORES'], function(e,d){
		console.log('zrange', e, d);
	});
	
	r.rawCall(['hgetall', 'hset:1'], function(e,d,size){
		console.log('hgetall', e, d, size);
	});
	
	r.rawCall(['SCAN', 0], function(e,d){
		console.log('scan 0', e, d);
	});
	
	r.rawCall(['HSCAN', 'hset:1', 0], function(e,d){
		console.log('hscan 0', e, d);
	});
	
	r.rawCall(['SET', 'kv:long', Array(1024 + 1).join('.')], function(e,d){
		r.rawCall(['GET', 'kv:long'], function(e,d,size){
			console.log('sizeof long key is %s', size); //1024
		});
	});
	
	if(--pings > 0)
		setTimeout(ping, 1000);
	else
		setTimeout(bench, 1000, [2500, 5000, 10000, 25000, 50000]);
}

var tests = [
	{
		description: 'PING command',
		cmd: ['PING']
	},
	{
		description: 'INCR command',
		cmd: ['INCR', 'INCR:TMP']
	},
	{
		description: 'GET command',
		cmd: ['GET', 'INCR:TMP']
	},
	{
		description: 'HGET command',
		cmd: ['HGET', 'hset:1', 'a']
	},
	{
		description: 'HGETALL command',
		cmd: ['HGETALL', 'hset:1']
	},
	{
		description: 'ZRANGE 0 4 command',
		cmd: ['ZRANGE', 'zset:1', 0, 4]
	}
];

function bench(repeats) {
	console.log(Array(50).join('='));
	var repeatIndex = 0;
	var i = 0;
	start();
	
	function start() {
		var repeat = repeats[repeatIndex++];
		if(!repeat) {
			console.log(Array(50).join('='));
			console.log('the end');
			r.end();
			return;
		}
		i=0;
		doIt(repeat, start);
	}
	
	function doIt(repeat, done) {
		var cmd = tests[i++];
		if(!cmd) {
			return done(); 
		}
		console.log('===\nStart test: %s %s times', cmd.description, repeat);
		var start = +new Date();
		for(var n=0;n<repeat;n++) {
			r.rawCall(cmd.cmd, onComplete);
		}
		
		var complete = 0;
		function onComplete(e, data){
			if(e) {
				console.log('error', e);
			}
			if(++complete === repeat) {
				var now = +new Date();
				var dt = now - start;
				console.log('Test complete in %sms, speed %s in second, cold down 1.5 sec', dt, (repeat/(dt/1000)).toFixed(2));
				setTimeout(doIt, 1500, repeat, done);
			}
		}
	}
}

ping();
