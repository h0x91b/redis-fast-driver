#ifndef REDIS_CLUSTER_H
#define REDIS_CLUSTER_H

#include <nan.h>

#define RFD_COMMAND_BUFFER_SIZE 4096;

#if ENABLELOG
#define LOG(...) fprintf( stderr, __VA_ARGS__ );
#else
#define LOG(...) do{ } while ( false )
#endif

#include <node.h>
#include <stdlib.h>
#include <unordered_map>
#include "../deps/hiredis/async.h"
#include "../deps/hiredis/hiredis.h"
#include "../deps/hiredis/adapters/libuv.h"

class RedisConnector : public node::ObjectWrap {
public:
	static void Init(v8::Handle<v8::Object> exports);
	Nan::Persistent<v8::Function> connectCb;
	Nan::Persistent<v8::Function> disconnectCb;
	Nan::Persistent<v8::Function> setImmediate;
	std::unordered_map< uint32_t, v8::Persistent<v8::Function> > callbacksMap;
	uint32_t callback_id;
	bool is_connected;

private:
	explicit RedisConnector();
	~RedisConnector();

	static NAN_METHOD(New);
	static NAN_METHOD(Connect);
	static NAN_METHOD(Disconnect);
	static NAN_METHOD(RedisCmd);
	static void ConnectCallback(const redisAsyncContext *c, int status);
	static void DisconnectCallback(const redisAsyncContext *c, int status);
	static void OnRedisResponse(redisAsyncContext *c, void *r, void *privdata);
	static Nan::Persistent<v8::Function> constructor;
	redisAsyncContext *c;
};

#endif