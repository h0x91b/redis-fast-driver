var Redis = require('../');

var redis = new Redis({host:'127.0.0.1', port: 7001, onConnect: onConnect});

function onConnect() {
	console.log('connected');
	console.log(redis.keyToSlot('123456789').toString(16));
	redis.cmd(['get','hello'], function(e, d){
		console.log(e,d);
	});
}

setInterval(function(){}, 1000);