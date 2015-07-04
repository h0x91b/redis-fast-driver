#ifndef REDIS_CLUSTER_H
#define REDIS_CLUSTER_H

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
	v8::Persistent<v8::Function> connectCb;
	v8::Persistent<v8::Function> disconnectCb;
	v8::Persistent<v8::Object> callbacks;
	uint32_t callback_id;
	double value_;
	bool is_connected;

private:
	explicit RedisConnector(double value = 0);
	~RedisConnector();

	static v8::Handle<v8::Value> New(const v8::Arguments& args);
	static v8::Handle<v8::Value> Connect(const v8::Arguments& args);
	static v8::Handle<v8::Value> Disconnect(const v8::Arguments& args);
	static v8::Handle<v8::Value> RedisCmd(const v8::Arguments& args);
	static void connectCallback(const redisAsyncContext *c, int status);
	static void disconnectCallback(const redisAsyncContext *c, int status);
	static void getCallback(redisAsyncContext *c, void *r, void *privdata);
	static v8::Persistent<v8::Function> constructor;
	redisAsyncContext *c;
};

#endif