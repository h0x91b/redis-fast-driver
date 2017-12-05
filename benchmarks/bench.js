'use strict';
/* eslint no-console:0 */

var childProcess = require('child_process');
var Redis = require('../');

console.log('==========================');
console.log(require('../package.json').name+': ' + require('../package.json').version);
var os = require('os');
console.log('CPU: ' + os.cpus().length);
console.log('OS: ' + os.platform() + ' ' + os.arch());
console.log('node version: ' + process.version);
console.log('current commit: ' + childProcess.execSync('git rev-parse --short HEAD'));
console.log('==========================');

var redis;
var waitReady = function (next) {
  var pending = 1;
  function check() {
    if (!--pending) {
      next();
    }
  }
  redis = new Redis({ host: '127.0.0.1', port: 6379});
  redis.on('error', console.error);
  redis.on('ready', check);
};

var quit = function () {
  redis.end();
};

var tests = [
  ['PING'],
  ['SET', 'foo', 'bar'],
  ['GET', 'foo'],
  ['INCR', 'number'],
  ['HGETALL', 'hset:1'],
  ['ZRANGE', 'zset:1', 0, 5],
  ['LRANGE', 'list', 0, 99]
];

var concurrents = [10000, 1000, 500, 250, 100, 10, 1];

concurrents.forEach((num) => {
  suite('Concurrency '+num, function(){
    //set('iterations', 100000);
    set('mintime', 5000);
    set('concurrency', num);
    set('delay', 3000);
    before(function (start) {
      waitReady(function(){
        var remain = 0;
        remain++, redis.rawCall(['HMSET', 'hset:1', 'a', 1, 'b', 2, 'c', 3], onDone);
        remain++, redis.rawCall(['ZADD', 'zset:1', 1, 'a', 2, 'b', 3, 'c', 4, 'd', 5, 'e'], onDone);
        remain++, (()=>{
          var item = [];
          for (var i = 0; i < 100; ++i) {
            item.push((Math.random() * 100000 | 0) + 'str');
          }
          redis.rawCall(['del', 'list']);
          redis.rawCall(['lpush', 'list'].concat(item), onDone);
        })();
        function onDone(e) {
          if(e) throw new Error(e);
          if(--remain === 0) {
            start();
          }
        }
      });
    });

    tests.forEach((test) => {
      bench(test.join(' '), function (next) {
        redis.rawCall(test, next);
      });
    });

    after(quit);
  });
});
