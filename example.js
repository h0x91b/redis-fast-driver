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

var pings = 3;

function ping() {
	r.rawCall(['PING'], function(e, d){
		console.log('PING', e, d);
	});
	
	r.rawCall(['hmget', 'hset:1', 'a', 'b', 'c', 'f'], function(e,d){
		console.log('hmget', e, d);
	});
	
	r.rawCall(['hgetall', 'hset:1'], function(e,d){
		console.log('hgetall', e, d);
	});
	
	if(--pings > 0)
		setTimeout(ping, 1000);
	else
		bench(0);
}

var benches = [1000, 10000, 50000, 50000, 50000, 50000, 50000];

function bench(test) {
	var toDo = benches[test];
	if(!toDo) {
		console.log('The End');
		r.end();
		return;
	}
	var start = null;
	r.rawCall(['DEL', 'INCR:TMP'], function(e){
		console.log('Begin bench');
		start = +new Date(); //timestamp with ms
		for(var i=0;i<toDo;i++) {
			r.rawCall(['INCR', 'INCR:TMP'], onComplete);
		}
	});
	
	function onComplete(e, result) {
		if(e) throw new Error(e);
		if(result >= toDo) {
			console.log('%s INCR completed in %s ms, speed %s in second', toDo, +new Date() - start, (result/((+new Date() - start)/1000)).toFixed(2));
			setTimeout(bench, 1000, ++test);
		}
	}
}

ping();
