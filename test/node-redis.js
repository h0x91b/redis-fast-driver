var redis = require("redis"), client = redis.createClient();

function test() {
	var now = +new Date;
	var toDo = 50000;
	var remain = toDo;
	for(var i=0;i<toDo;i++) {
		client.set('hello'+i, 'world'+i, function(e, d) {
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