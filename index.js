var redis = require('./build/Release/redis-cluster');

var connector = new redis.RedisConnector();
connector.connect('127.0.0.1', '7001', function(e){
	if(e) throw e;
	console.log('connect callback ', arguments);
	
	function test(){
		var toDo = 100000;
		var now = +new Date;
		for(var i=0;i<toDo;i++){
			connector.redisCmd(['set', 'hello11', 'world'+i], function(err, data){
				if(err) {
					console.log(err);
					process.exit();
					throw err;
				}
				//console.log('set', err, data);
				--toDo;
				if(toDo<=0) {
					var t = (+new Date - now);
					var speed = (100000/t*1000).toFixed(2);
					console.log('done in %s ms speed %s / sec', t, speed);
					setTimeout(test, 10);
				}
			});
		}
	}
	test();
		
	connector.redisCmd(['get', 'hello11'], function(err, data){
		console.log('get', err, data);
	});
	
	connector.redisCmd(['set', 'hello2', 'world'], function(err, data){
		console.log('set', err, data);
	});
});
setInterval(function(){}, 1000);