const Redis = require('.');
const {Worker, isMainThread} = require('worker_threads');

if (isMainThread) {
  console.log('Main Thread');
  const worker = new Worker(__filename);
  worker.on('message', console.log);
} else {
  console.log(`in worker ${__filename}`);
  worker();
}
console.log('first pass done');

function worker() {
	var r = new Redis({
		//host: '/tmp/redis.sock', //unix domain
		host: '127.0.0.1', //can be IP or hostname
		port: 6379,
		maxretries: 10, //reconnect retries, default -1 (infinity)
		// auth: '123',
		// db: 5
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
	
	setTimeout(()=>{}, 5000);
}