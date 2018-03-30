'use strict';

const Redis = require('../');
const Promise = require('bluebird');
const assert = require('power-assert');

describe('redis-fast-driver', function() {
  let redis;

  afterEach(() => {
    if (redis) redis.end();
  });

  function eventPromise(event) {
    return new Promise((resolve) => redis.once(event, resolve));
  }
  function rawCall(args) {
    return Promise.fromCallback((cb) => redis.rawCall(args, cb));
  }

  describe('Connecting', function() {
    it('Connects', async function() {
      redis = new Redis();
      const readyPromise = eventPromise('ready');
      const connectPromise = eventPromise('connect');
      await Promise.all([readyPromise, connectPromise]);
    });

    it('Does not connect when autoConnect:false', async function() {
      redis = new Redis({autoConnect: false});
      const readyPromise = eventPromise('ready');
      const connectPromise = eventPromise('connect');

      let isManuallyConnected = false;
      const prematurePromise = Promise.all([readyPromise, connectPromise])
      .then(() => {
        if (!isManuallyConnected) throw new Error('Connected too early!');
      });

      const connPromise = Promise.delay(100)
      .then(() => {
        isManuallyConnected = true;
        redis.connect();
      });

      await Promise.all([prematurePromise, connPromise]);
      assert(redis.ready);
      assert(redis.readyFirstTime);
    });

    it('Can be destroyed and revived', async function() {
      redis = new Redis({
        doNotRunQuitOnEnd: true,
        doNotSetClientName: true,
      });
      await eventPromise('ready');

      redis.end();
      await eventPromise('end');
      assert(!redis.ready);
      assert(redis.destroyed);

      redis.init();
      await eventPromise('ready');
      assert(redis.ready);
      assert(redis.readyFirstTime);
      assert(!redis.destroyed);
    });

    it('Reconnects, and emits "end" at limit', async function() {
      const maxRetries = 10;
      redis = new Redis({
        maxRetries,
        reconnectTimeout: 0,
        doNotSetClientName: true,
        doNotRunQuitOnEnd: true
      });
      // redis.on('connect', () => console.log('connect'));
      // redis.on('reconnecting', (num) => console.log('reconn', num));
      // redis.on('disconnect', () => console.log('disconnect'));
      // redis.on('end', () => console.log('end'));
      redis.on('error', () => {});

      const endPromise = eventPromise('end');

      // Monkey-patch as not to reset the reconnect timer
      redis.onConnect = function() {
        redis.emit('connect');
      };

      await eventPromise('connect');

      // Disconnect limit + 1 times
      await Promise.mapSeries(Array(maxRetries).fill(0), () => {
        const events = Promise.all([eventPromise('disconnect'), eventPromise('connect')]);
        redis.redis.disconnect();
        return events;
      });

      assert(redis.reconnects === maxRetries);

      // Once more to kill it
      redis.redis.disconnect();
      await endPromise;
    });

    it('Disconnects if ended during connect (potential race condition)', async function() {
      redis = new Redis({autoConnect: false,
        doNotSetClientName: true,
        doNotRunQuitOnEnd: true
      });

      redis.redis.connect = function(host, port, onConnect, onDisconnect) {
        setImmediate(onConnect);
      };
      let disconnected = false;
      redis.redis.disconnect = function() {
        disconnected = true;
      };

      redis.connect();
      redis.end();
      assert(redis.destroyed);
      assert(disconnected === false);
      await Promise.delay(0);

      // onConnect() called, realizes destroyed, calls redis.disconnect
      assert(disconnected === true);
    });
  });

  describe('rawCall', function() {
    beforeEach(async function() {
      redis = new Redis();
      await eventPromise('connect');
    });

    it('ping', async function() {
      const resp = await rawCall(['ping']);
      assert(resp === 'PONG');
    });

    it('ping (rawCallAsync)', async function() {
      const resp = await redis.rawCallAsync(['ping']);
      assert(resp === 'PONG');
    });

    it('set/get', async function() {
      const set = await rawCall(['set', 'number', 123]);
      assert(set === 'OK');
      const get = await rawCall(['get', 'number']);
      assert(get === '123');
    });

    it('incr', async function() {
      const incr = await rawCall(['incr', 'number']);
      assert(incr === 124); // note number type
    });

    it('zrange', async function() {
      const key = 'sortedset'+new Date;
      const zadd = await rawCall(['zadd', key, 1, 'a', 2, 'b', 3, 'c']);
      assert(zadd === 3);
      const zrange = await rawCall(['zrange', key, 0, -1]);
      assert.deepEqual(zrange, ['a', 'b', 'c']);
    });
    
    it('del hset:1', async function() {
      await rawCall(['del', 'hset:1']);
      assert(1 === 1);
    });
    
    it('hmset', async function() {
      const hmset = await rawCall(['hmset', 'hset:1', 'a', 1, 'b', 2, 'c', 3]);
      assert(hmset === 'OK');
    });

    it('hscan', async function() {
      const hscan = await rawCall(['hscan', 'hset:1', 0]);
      assert.deepEqual(hscan, ['0', ['a', '1', 'b', '2', 'c', '3']]);
    });

    it('hgetall', async function() {
      const hgetall = await rawCall(['hgetall', 'hset:1']);
      assert.deepEqual(hgetall, ['a', '1', 'b', '2', 'c', '3']);
    });
    
    it('del zset', async function() {
      const zadd = await rawCall(['del', 'zset:1']);
      assert(1 === 1);
    });

    it('zadd', async function() {
      const zadd = await rawCall(['zadd', 'zset:1', 1, 'a', 2, 'b', 3, 'c', 4, 'd', 5, 'e']);
      assert(zadd === 5);
    });

    it('zrange', async function() {
      const zrange = await rawCall(['zrange', 'zset:1', 0, -1]);
      assert.deepEqual(zrange, ['a', 'b', 'c', 'd', 'e']);
    });

    it('works correctly when send buffer needs to be resized', async function() {
      // This should be the same as RFD_COMMAND_BUFFER_SIZE defined in
      // redis-fast-driver.h
      const commandBufferSize = 4096;
      let bigBuff;
      for(let i=1;i<8;i++) {
        //multiply * i for exceeding the buffer size, so buf is 4096*i '+'
        bigBuff = Array(commandBufferSize*i+1).join('+');
        await rawCall(['SET', 'key', bigBuff]);
      }
      const result = await rawCall(['GET', 'key']);
      assert.ok(bigBuff === result);
    });
    
    it('works correctly when command buffer needs to be resized', async function() {
      const cmd = ['HMSET', 'hset:1'];
      for(let i=0;i<=128;i++) {
        cmd.push('key'+i, 'val'+i);
      }
      await rawCall(cmd);
      const len = await rawCall(['HLEN', 'hset:1']);
      assert.ok(len > 128);
    });

    describe('errors', function() {

      it('incr a string', async function() {
        await rawCall(['set', 'str', 'hello']);
        try {
          await rawCall(['incr', 'str']);
          assert(false);
        } catch (e) {
          assert(/not an integer or out of range/.test(e.message));
        }
      });

      it('zrange string', async function() {
        try {
          await rawCall(['zrange', 'zset:1', 1, 'a', 2, 'b', 3, 'c', 4, 'd']);
          assert(false);
        } catch (e) {
          assert(/not an integer or out of range/.test(e.message));
        }
      });
    });

  });

  describe('queueing', function() {
    beforeEach(async function() {
      redis = new Redis({
        autoConnect: false,
        doNotSetClientName: true,
        doNotRunQuitOnEnd: true
      });
    });

    it('Queues up messages before connect', async function() {
      const cmds = [['ping'], ['set', 'number', '1'], ['get', 'number']];
      const promises = cmds.map((cmd) => rawCall(cmd));

      assert(redis.queue.length === cmds.length);
      cmds.forEach((val, idx) => {
        assert.deepEqual(redis.queue[idx].args, val);
      });

      redis.connect();
      await eventPromise('connect');

      const results = await Promise.all(promises);
      assert.deepEqual(results, ['PONG', 'OK', '1']);
      assert(redis.queue.length === 0);
    });

    it('Releases queue on end', async function() {
      const cmds = [['ping'], ['set', 'number', '1'], ['get', 'number']];
      cmds.map((cmd) => rawCall(cmd));

      assert(redis.queue.length === cmds.length);
      redis.end();
      assert.deepEqual(redis.queue, []);
    });
  });
});
