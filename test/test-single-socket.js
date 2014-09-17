var Redis = require('../');
var redis = new Redis({
	host:'/tmp/redis.sock', 
	onConnect: onConnect,
	onError: function(e) {
		console.log('redis error: ', e);
		throw e;
	}
});

redis.cmd(['set', 'queue_check', 'this queue value queued before redis actually connected']);
redis.cmd(['get', 'queue_check'], function(e, data){
	if(e) throw e;
	console.log('queue is working, value: '+data);
});
redis.cmd(['del', 'queue_check']);
redis.cmd(['ping'], function(e,d){
	console.log(d);
});

redis.disconnect();

redis.cmd(['hmset', 'somehset', 'a', '777', 'b', '888']);
redis.cmd(['hgetall', 'somehset'], function(e,d){
	console.log(d);
});

// redis.cmd(['subscribe', 'hello channel'], function(e,d){
// 	console.log(d);
// });

function onConnect() {
	console.log('adapter ready');
	for(var i=0;i<1000;i++){
		redis.cmd(['set', 'hello'+i, Math.random()]);
	}
	for(var i=0;i<10;i++){
		redis.cmd(['get', 'hello'+i], function(e, d) {
			console.log(e,d);
		});
	}
	
	function test() {
		var now = +new Date;
		var toDo = 50000;
		var remain = toDo;
		for(var i=0;i<toDo;i++) {
			redis.cmd(['set', 'hello'+i,'world'+i], function(e, d) {
				if(e) throw e;
				if(--remain == 0) {
					var dt = +new Date - now;
					var speed = (toDo / dt * 1000).toFixed(2);
					console.log(toDo+' ops in %s ms, speed %s ops/sec', dt, speed);
					setTimeout(test, 100);
				}
			});
		}
	}
	
	setTimeout(test, 500);
}

setInterval(function(){}, 1000);