var Redis = require('../');
var redis = new Redis({host:'127.0.0.1', port: 7001, onConnect: onConnect});

function onConnect() {
	console.log('cluster ready');
	for(var i=0;i<1000;i++){
		redis.cmd(['set', 'hello'+i, Math.random()]);
	}
	for(var i=0;i<10;i++){
		redis.cmd(['get', 'hello'+i], function(e, d) {
			console.log(e,d);
		});
	}
}

setInterval(function(){}, 1000);