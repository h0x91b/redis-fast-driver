Redis-fast-driver
===

Trully async redis driver. Extremly simple, extremely fast.

This node module use hiredis async library written on C by Salvatore Sanfilippo.

All regular functions including PUB/SUB and MONITOR mode works fine, this driver almost an year on my production enviroment under really high load (30k ops/sec each redis in cluster)...
Also this driver used in https://github.com/joaojeronimo/node_redis_cluster and in my fork https://github.com/h0x91b/fast-redis-cluster

Usage
===

Look at `example*.js` for usage.

Speed
===

Works MUCH faster then node-redis and even faster then `redis-benchmark` tool.

Results for my MacBook Pro (Retina, 15-inch, Mid 2014) via tcp/ip.

My driver:

	=================================================
	===
	Start test: PING command 1000 times
	Test complete in 7ms, speed 142857.14 in second, cold down 1.5 sec
	===
	Start test: INCR command 1000 times
	Test complete in 6ms, speed 166666.67 in second, cold down 1.5 sec
	===
	Start test: GET command 1000 times
	Test complete in 6ms, speed 166666.67 in second, cold down 1.5 sec
	===
	Start test: HGET command 1000 times
	Test complete in 7ms, speed 142857.14 in second, cold down 1.5 sec
	===
	Start test: HGETALL command 1000 times
	Test complete in 8ms, speed 125000.00 in second, cold down 1.5 sec
	===
	Start test: ZRANGE 0 4 command 1000 times
	Test complete in 12ms, speed 83333.33 in second, cold down 1.5 sec
	===
	Start test: PING command 5000 times
	Test complete in 15ms, speed 333333.33 in second, cold down 1.5 sec
	===
	Start test: INCR command 5000 times
	Test complete in 16ms, speed 312500.00 in second, cold down 1.5 sec
	===
	Start test: GET command 5000 times
	Test complete in 18ms, speed 277777.78 in second, cold down 1.5 sec
	===
	Start test: HGET command 5000 times
	Test complete in 21ms, speed 238095.24 in second, cold down 1.5 sec
	===
	Start test: HGETALL command 5000 times
	Test complete in 35ms, speed 142857.14 in second, cold down 1.5 sec
	===
	Start test: ZRANGE 0 4 command 5000 times
	Test complete in 32ms, speed 156250.00 in second, cold down 1.5 sec
	===
	Start test: PING command 10000 times
	Test complete in 28ms, speed 357142.86 in second, cold down 1.5 sec
	===
	Start test: INCR command 10000 times
	Test complete in 26ms, speed 384615.38 in second, cold down 1.5 sec
	===
	Start test: GET command 10000 times
	Test complete in 29ms, speed 344827.59 in second, cold down 1.5 sec
	===
	Start test: HGET command 10000 times
	Test complete in 32ms, speed 312500.00 in second, cold down 1.5 sec
	===
	Start test: HGETALL command 10000 times
	Test complete in 62ms, speed 161290.32 in second, cold down 1.5 sec
	===
	Start test: ZRANGE 0 4 command 10000 times
	Test complete in 63ms, speed 158730.16 in second, cold down 1.5 sec
	===
	Start test: PING command 25000 times
	Test complete in 76ms, speed 328947.37 in second, cold down 1.5 sec
	===
	Start test: INCR command 25000 times
	Test complete in 79ms, speed 316455.70 in second, cold down 1.5 sec
	===
	Start test: GET command 25000 times
	Test complete in 97ms, speed 257731.96 in second, cold down 1.5 sec
	===
	Start test: HGET command 25000 times
	Test complete in 99ms, speed 252525.25 in second, cold down 1.5 sec
	===
	Start test: HGETALL command 25000 times
	Test complete in 163ms, speed 153374.23 in second, cold down 1.5 sec
	===
	Start test: ZRANGE 0 4 command 25000 times
	Test complete in 159ms, speed 157232.70 in second, cold down 1.5 sec
	=================================================

Redis benchmark tool:

	PING_INLINE: 125313.29 requests per second
	PING_BULK: 125313.29 requests per second
	SET: 126742.72 requests per second
	GET: 124533.01 requests per second
	INCR: 126742.72 requests per second
	LPUSH: 126422.25 requests per second
	LPOP: 125000.00 requests per second
	SADD: 121951.22 requests per second
	SPOP: 125628.14 requests per second
	LPUSH (needed to benchmark LRANGE): 117785.63 requests per second
	LRANGE_100 (first 100 elements): 31948.88 requests per second
	LRANGE_300 (first 300 elements): 13264.36 requests per second
	LRANGE_500 (first 450 elements): 9039.96 requests per second
	LRANGE_600 (first 600 elements): 6799.95 requests per second
	MSET (10 keys): 87873.46 requests per second

Author
===

Arseniy Pavlenko h0x91b@gmail.com

Skype: h0x91b

Linkedin: https://il.linkedin.com/in/h0x91b
