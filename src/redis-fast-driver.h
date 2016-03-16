#ifndef REDIS_CLUSTER_H
#define REDIS_CLUSTER_H

#include <nan.h>

#if ENABLELOG
#define LOG(...) fprintf( stderr, __VA_ARGS__ );
#else
#define LOG(...) do{ } while ( false )
#endif

#include <node.h>
#include <stdlib.h>
#include "../deps/hiredis/async.h"
#include "../deps/hiredis/hiredis.h"
#include "../deps/hiredis/adapters/libuv.h"

class RedisConnector : public node::ObjectWrap {
public:
	static void Init(v8::Handle<v8::Object> exports);
	Nan::Persistent<v8::Function> connectCb;
	Nan::Persistent<v8::Function> disconnectCb;
	Nan::Persistent<v8::Function> setImmediate;
	Nan::Persistent<v8::Object> callbacks;
	uint32_t callback_id;
	double value_;
	bool is_connected;

private:
	explicit RedisConnector(double value = 0);
	~RedisConnector();

	static NAN_METHOD(New);
	static NAN_METHOD(Connect);
	static NAN_METHOD(Disconnect);
	static NAN_METHOD(RedisCmd);
	static void connectCallback(const redisAsyncContext *c, int status);
	static void disconnectCallback(const redisAsyncContext *c, int status);
	static void getCallback(redisAsyncContext *c, void *r, void *privdata);
	static Nan::Persistent<v8::Function> constructor;
	redisAsyncContext *c;
};

#endif