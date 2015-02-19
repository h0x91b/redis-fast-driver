var Redis = require('./index.js');

var r = new Redis({
	host: '127.0.0.1',
	port: 6379
});

r.on('ready', function(){
	console.log('redis connected');
});

r.on('error', function(e){
	console.log('redis error', e);
});

r.rawCall(['hmset', 'hset:1', 'a', 1, 'b', 2, 'c', 3]);

function ping() {
	r.rawCall(['PING'], function(e, d){
		console.log('PING', e, d);
	});
	
	r.rawCall(['hmget', 'hset:1', 'a', 'b', 'c', 'f'], function(e,d){
		console.log('hmget', e, d);
	});
	
	setTimeout(ping, 1000);
}

ping();
