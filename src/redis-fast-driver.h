#ifndef REDIS_CLUSTER_H
#define REDIS_CLUSTER_H

#include <napi.h>
#include <uv.h>

#define RFD_COMMAND_BUFFER_SIZE 4096;

#if ENABLELOG
#define LOG(...) fprintf( stderr, __VA_ARGS__ );
#else
#define LOG(...) do{ } while ( false )
#endif

// #define NAPI_VERSION 3
#include <node_api.h>
// #include <napi.h>
#include <uv.h>
#include <stdlib.h>
#include <unordered_map>
#include "../deps/hiredis/async.h"
#include "../deps/hiredis/hiredis.h"
#include "../deps/hiredis/adapters/libuv.h"

class RedisConnector /*: public Napi::ObjectWrap<RedisConnector>*/ {
public:
	static void Init(Napi::Env env, napi_value exports);
	napi_callback connectCb;
	napi_callback disconnectCb;
	napi_callback setImmediate;
	std::unordered_map< uint32_t, Napi::FunctionReference > callbacksMap;
	uint32_t callback_id;
	bool is_connected;

private:
	//explicit RedisConnector();
	RedisConnector(const napi_callback_info info);
	~RedisConnector();
	static void Destructor(napi_env env, void* nativeObject, void* finalize_hint);

	static napi_value New(napi_env env, napi_callback_info info);
	static napi_value Connect(napi_env env, napi_callback_info info);
	static napi_value Disconnect(napi_env env, napi_callback_info info);
	static napi_value RedisCmd(napi_env env, napi_callback_info info);
	static void ConnectCallback(const redisAsyncContext *c, int status);
	static void DisconnectCallback(const redisAsyncContext *c, int status);
	static void OnRedisResponse(redisAsyncContext *c, void *r, void *privdata);
	redisAsyncContext *c;
	
	static napi_ref constructor;
	napi_env env_;
	napi_ref wrapper_;
};

#endif