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

r.rawCall(['ping'], function(e, resp){
	console.log('ping', e, resp);
});

r.rawCall(['hmset', 'hset:1', 'a', 1, 'b', 2, 'c', 3], function(e, resp){
	console.log('hmset', e, resp);
});

r.rawCall(['hscan', 'hset:1', 0], function(e, resp){
	console.log('hscan 0', e, resp);
});

r.rawCall(['zadd', 'zset:1', 1, 'a', 2, 'b', 3, 'c', 4, 'd'], function(e, resp){
	console.log('zset', e, resp);
});

r.rawCall(['hgetall', 'hset:1'], function(e, resp){
	console.log('HGETALL', e, resp);
});

r.rawCall(['zrange', 'zset:1', 0, -1], function(e, resp){
	console.log('ZRANGE', e, resp);
});