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

r.rawCall(['subscribe', 'foo'], function(e, data){
	console.log('foo', data);
});

r.rawCall(['subscribe', 'foo2'], function(e, data){
	console.log('foo2', data);
});

var r2 = new Redis({
	host: '127.0.0.1',
	port: 6379
});

setInterval(function(){
	r2.rawCall(['PUBLISH', 'foo', 'bar']);
}, 1000);

setInterval(function(){
	r2.rawCall(['PUBLISH', 'foo2', 'bar2']);
}, 3000);