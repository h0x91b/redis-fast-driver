#ifndef BUILDING_NODE_EXTENSION
	#define BUILDING_NODE_EXTENSION
#endif
#include <math.h>
#include <napi.h>
#include <uv.h>
#include <node_version.h>
#include "redis-fast-driver.h"

using namespace Napi;

// Napi::Object init(Napi::Env env, Napi::Object exports) {
// 	RedisConnector::Init(env, target, module);
// }

//NODE_API_MODULE(redis_fast_driver, init)
// NODE_API_MODULE_INIT(/* exports, module, context */) {
// 	Init(exports, context);
// }

// NODE_API_MODULE_INIT(/* exports, module, context */) {
// 	RedisConnector::Init(exports, module, context);
// }
//

NAPI_MODULE_INIT(/*env, exports*/) {
	RedisConnector::Init(env, exports);
	return exports;
}

// NAPI_MODULE(NODE_GYP_MODULE_NAME, Init)

RedisConnector::RedisConnector(const napi_callback_info info) : /*Napi::ObjectWrap<RedisConnector>(info), */env_(nullptr), wrapper_(nullptr) {
	LOG("%s\n", __PRETTY_FUNCTION__);
	callback_id = 1;
}

RedisConnector::~RedisConnector() {
	LOG("%s\n", __PRETTY_FUNCTION__);
	napi_delete_reference(env_, wrapper_);
}

#define DECLARE_NAPI_METHOD(name, func) \
	{ name, 0, func, 0, 0, 0, napi_default, 0 }

napi_ref RedisConnector::constructor;

void RedisConnector::Init(Napi::Env env, napi_value exports) {
	napi_status status;
	napi_property_descriptor properties[] = {
		DECLARE_NAPI_METHOD("connect", Connect),
		DECLARE_NAPI_METHOD("disconnect", Disconnect),
		DECLARE_NAPI_METHOD("redisCmd", RedisCmd),
	};

	napi_value cons;
	status = napi_define_class(env, "RedisConnector", NAPI_AUTO_LENGTH, New, nullptr, 1, properties, &cons);
	if (status != napi_ok) return;

	status = napi_create_reference(env, cons, 1, &constructor);
	if (status != napi_ok) return;

	return;
	
	//
	// Napi::HandleScope scope(env);
	// Napi::Function cls = DefineClass(env, "RedisConnector", {
	// 	StaticMethod("connect", &RedisConnector::Connect),
	// 	StaticMethod("disconnect", &RedisConnector::Disconnect),
	// 	StaticMethod("redisCmd", &RedisConnector::RedisCmd),
	// });
	//
	// constructor = Napi::Persistent(cls);
	// constructor.SuppressDestruct();
	// //exports.Set("RedisConnector", cls);
	// napi_set_named_property(env, exports, "RedisConnector", cls);
}

// void RedisConnector::Init(Napi::Object exports) {
// 	Napi::HandleScope scope(env);
// 	Napi::FunctionReference tpl = Napi::Function::New(env, New);
// 	tpl->SetClassName(Napi::String::New(env, "RedisConnector"));

// 	// Prototype
// 	Napi::SetPrototypeTemplate(tpl, "connect", Napi::Function::New(env, Connect));
// 	Napi::SetPrototypeTemplate(tpl, "disconnect", Napi::Function::New(env, Disconnect));
// 	Napi::SetPrototypeTemplate(tpl, "redisCmd", Napi::Function::New(env, RedisCmd));
//
// 	constructor.Reset(Napi::GetFunction(tpl));
// 	(
// 		exports).Set(// 		Napi::String::New(env, "RedisConnector"),
// 		Napi::GetFunction(tpl)
// 	);
// }

napi_value RedisConnector::New(napi_env env, napi_callback_info info) {
	napi_status status;

	size_t argc = 1;
	napi_value args[1];
	napi_value jsthis;
	status = napi_get_cb_info(env, info, &argc, args, &jsthis, nullptr);
	
	if(status != napi_ok) {
		return 0;
	}

	// napi_valuetype valuetype;
	// status = napi_typeof(env, args[0], &valuetype);
	// assert(status == napi_ok);
	//
	RedisConnector* obj = new RedisConnector(info);
	//
	// if (valuetype == napi_undefined) {
	// 	obj->counter_ = 0;
	// } else {
	// 	status = napi_get_value_double(env, args[0], &obj->counter_);
	// 	assert(status == napi_ok);
	// }

	obj->env_ = env;
	status = napi_wrap(env,
		jsthis,
		reinterpret_cast<void*>(obj),
		RedisConnector::Destructor,
		nullptr, /* finalize_hint */
		&obj->wrapper_);
	
	if(status != napi_ok) {
		return 0;
	}

	return jsthis;
	
	
// Napi::HandleScope scope(env);
//
// 	if (info.IsConstructCall()) {
// 		// Invoked as constructor: `new RedisConnector(...)`
// 		RedisConnector* obj = new RedisConnector(info);
// 		obj->Wrap(info.This());
// 		return info.This();
// 	} else {
// 		// Invoked as plain function `RedisConnector(...)`, turn into construct call.
// 		Napi::Error::New(env, "This function must be called as a constructor (e.g. new RedisConnector())").ThrowAsJavaScriptException();
// 	}
}

void RedisConnector::ConnectCallback(const redisAsyncContext *c, int redisStatus) {
	LOG("%s\n", __PRETTY_FUNCTION__);
	RedisConnector *self = (RedisConnector*)c->data;
	Napi::HandleScope scope(self->env_);
	
	napi_value global, errstr, fnConnectCb, nullObj, return_val;
	napi_status status = napi_get_global(self->env_, &global);
	if (status != napi_ok) return;
	
	status = napi_create_function(self->env_, NULL, 0, self->connectCb, NULL, &fnConnectCb);
	if (status != napi_ok) return;
	
	if (redisStatus != REDIS_OK) {
		LOG("%s !REDIS_OK\n", __PRETTY_FUNCTION__);
		self->is_connected = false;
		
		napi_create_string_utf8(self->env_, c->errstr, strlen(c->errstr), &errstr);
		napi_value argv[1] = {
			errstr
		};
		
		napi_call_function(self->env_, global, fnConnectCb, 1, argv, &return_val);
		return;
	}
	self->is_connected = true;
	
	napi_get_null(self->env_, &nullObj);
	napi_value argv[1] = {
		nullObj
	};
	napi_call_function(self->env_, global, fnConnectCb, 1, argv, &return_val);
}

void RedisConnector::DisconnectCallback(const redisAsyncContext *c, int redisStatus) {
	LOG("%s\n", __PRETTY_FUNCTION__);
	RedisConnector *self = (RedisConnector*)c->data;
	Napi::HandleScope scope(self->env_);
	
	napi_value global, errstr, fnDisconnectCb, nullObj, return_val;
	napi_status status = napi_get_global(self->env_, &global);
	if (status != napi_ok) return;
	
	status = napi_create_function(self->env_, NULL, 0, self->disconnectCb, NULL, &fnDisconnectCb);
	if (status != napi_ok) return;
	
	self->is_connected = false;
	
	if (redisStatus != REDIS_OK) {
		napi_create_string_utf8(self->env_, c->errstr, strlen(c->errstr), &errstr);
		napi_value argv[1] = {
			errstr
		};
		
		napi_call_function(self->env_, global, fnDisconnectCb, 1, argv, &return_val);
		return;
	}
	
	napi_get_null(self->env_, &nullObj);
	napi_value argv[1] = {
		nullObj
	};
	napi_call_function(self->env_, global, fnDisconnectCb, 1, argv, &return_val);
}

napi_value RedisConnector::Disconnect(napi_env env, napi_callback_info info) {
	LOG("%s\n", __PRETTY_FUNCTION__);
	Napi::HandleScope scope(env);
	
	napi_status status;
	napi_value jsthis, undefined;
	
	status = napi_get_cb_info(env, info, nullptr, nullptr, &jsthis, nullptr);
	RedisConnector* self;
	status = napi_unwrap(env, jsthis, reinterpret_cast<void**>(&self));
	//RedisConnector* self = ObjectWrap::Unwrap<RedisConnector>(info.This());
	
	if(self->c->replies.head != NULL) {
		LOG("there is more callbacks in queue...\n");
	}
	if(self->is_connected) redisAsyncDisconnect(self->c);
	self->is_connected = false;
	self->c = NULL;
	napi_get_undefined(env, &undefined);
	return undefined;
}

napi_value RedisConnector::Connect(napi_env env, napi_callback_info info) {
	LOG("%s\n", __PRETTY_FUNCTION__);
	Napi::HandleScope scope(env);
	if(info.Length() != 4) {
		Napi::Error::New(env, "Wrong arguments count").ThrowAsJavaScriptException();

		return env.Undefined();
	}
	RedisConnector* self = ObjectWrap::Unwrap<RedisConnector>(info.This());
	
	std::string v8str = info[0].As<Napi::String>();
	const char *host = *v8str;
	unsigned short port = (unsigned short)info[1].Uint32Value(Napi::GetCurrentContext()).ToChecked();
	Napi::Function connectCb = info[2].As<Napi::Function>();
	self->connectCb.Reset(connectCb);
	Napi::Function disconnectCb = info[3].As<Napi::Function>();
	self->disconnectCb.Reset(disconnectCb);
	Napi::Function setImmediate = Napi::Function::Cast(
		(Napi::GetCurrentContext()->Global()).Get(Napi::String::New(env, "setImmediate")
	));
	self->setImmediate.Reset(setImmediate);
	
	if(strstr(host,"/")==host) {
		LOG("connect to unix:%s\n", host);
		self->c = redisAsyncConnectUnix(host);
	} else {
		LOG("connect to %s:%d\n", host, port);
		self->c = redisAsyncConnect(host, port);
	}
	if (self->c->err) {
		self->is_connected = false;
		Napi::Value argv[1] = {
			Napi::String::New(env, self->c->errstr)
		};
		Napi::Call(Napi::New(env, self->connectCb), Napi::GetCurrentContext()->Global(), 1, argv);
		return env.Undefined();
		return;
	}
	uv_loop_t* loop = uv_default_loop();
	self->c->data = (void*)self;
	_env = env;
	redisLibuvAttach(self->c,loop);
	redisAsyncSetConnectCallback(self->c,ConnectCallback);
	redisAsyncSetDisconnectCallback(self->c,DisconnectCallback);
	
	return env.Undefined();
}

Napi::Value ParseResponse(redisReply *reply, size_t* size) {
	Napi::EscapableHandleScope scope(env);
	Napi::Value resp;
	Napi::Array arr = Napi::Array::New(env);
	
	switch(reply->type) {
	case REDIS_REPLY_NIL:
		resp = env.Null();
		*size += sizeof(NULL);
		break;
	case REDIS_REPLY_INTEGER:
		resp = Napi::Number::New(env, reply->integer);
		*size += sizeof(int);
		break;
	case REDIS_REPLY_STATUS:
	case REDIS_REPLY_STRING:
		resp = Napi::String::New(env, reply->str, reply->len);
		*size += reply->len;
		break;
	case REDIS_REPLY_ARRAY:
		for (size_t i=0; i<reply->elements; i++) {
			(arr).Set(Napi::Number::New(env, i), ParseResponse(reply->element[i], size));
		}
		resp = arr;
		break;
	default:
		printf("Redis rotocol error, unknown type %d\n", reply->type);
		Napi::Error::New(env, "Protocol error, unknown type").ThrowAsJavaScriptException();

		return env.Undefined();
	}
	
	return scope.Escape(resp);
}

void RedisConnector::OnRedisResponse(redisAsyncContext *c, void *r, void *privdata) {
	Napi::HandleScope scope(env);
	size_t totalSize = 0;
	//LOG("%s\n", __PRETTY_FUNCTION__);
	redisReply *reply = (redisReply*)r;
	uint32_t callback_id = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(privdata));
	if (reply == NULL) return;
	RedisConnector *self = (RedisConnector*)c->data;
	Napi::Function jsCallback = Napi::New(env, self->callbacksMap[callback_id]);
	Napi::FunctionReference cb(jsCallback);
	Napi::Function setImmediate = Napi::New(env, self->setImmediate);
	if(!(c->c.flags & REDIS_SUBSCRIBED || c->c.flags & REDIS_MONITORING)) {
		// LOG("delete, flags %i id %i\n", c->c.flags, callback_id);
		self->callbacksMap[callback_id].Reset();
		self->callbacksMap.erase(callback_id);
	} else {
		// LOG("flags %i id %i\n", c->c.flags, callback_id);
	}
	
	if (reply->type == REDIS_REPLY_ERROR) {
		//LOG("[%d] redis error: %s\n", callback_id, reply->str);
		totalSize += reply->len;
		Napi::Value argv[4] = {
			jsCallback,
			Napi::New(env, reply->str),
			env.Undefined(),
			Napi::Number::New(env, totalSize)
		};
		Napi::Call(setImmediate, Napi::GetCurrentContext()->Global(), 4, argv);
		return;
	}
	Napi::Value resp = ParseResponse(reply, &totalSize);
	if( resp->IsUndefined() ) {
		Napi::Value argv[4] = {
			jsCallback,
			Napi::String::New(env, "Protocol error, can not parse answer from redis"),
			env.Undefined(),
			Napi::Number::New(env, totalSize)
		};
		Napi::Call(setImmediate, Napi::GetCurrentContext()->Global(), 4, argv);
		return;
	}
	
	Napi::Value argv[4] = {
		jsCallback,
		env.Null(),
		resp,
		Napi::Number::New(env, totalSize)
	};
	Napi::Call(setImmediate, Napi::GetCurrentContext()->Global(), 4, argv);
}

napi_value RedisConnector::RedisCmd(napi_env env, napi_callback_info info) {
	//LOG("%s\n", __PRETTY_FUNCTION__);
	static size_t bufsize = RFD_COMMAND_BUFFER_SIZE;
	static char* buf = (char*)malloc(bufsize);
	static size_t argvroom = 128;
	static size_t *argvlen = (size_t*)malloc(argvroom * sizeof(size_t*));
	static char **argv = (char**)malloc(argvroom * sizeof(char*));
	
	size_t bufused = 0;
	Napi::HandleScope scope(env);
	if(info.Length() != 2) {
		Napi::Error::New(env, "Wrong arguments count").ThrowAsJavaScriptException();

		return env.Undefined();
	}
	RedisConnector* self = ObjectWrap::Unwrap<RedisConnector>(info.This());
	
	Napi::Array array = info[0].As<Napi::Array>();
	size_t arraylen = array->Length();

	if(arraylen > argvroom) {
		argvroom = (arraylen / 8 + 1) * 8;
		// LOG("increase room for argv to %zu\n", argvroom);
		
		// LOG("resizing argvlen/argv");
		free(argvlen);
		free(argv);
		argvlen = (size_t*)malloc(argvroom * sizeof(size_t*));
		argv = (char**)malloc(argvroom * sizeof(char*));
	}
	
	for(uint32_t i=0;i<arraylen;i++) {
		std::string str = (array).Get(i.As<Napi::String>());
		uint32_t len = str.Length();
		//LOG("i %u\n", i);
		//LOG("str: \"%s\"\n", *str);
		//LOG("len %u\n", len);
		//LOG("bufused %zu\n", bufused);
		if(bufused + len > bufsize) {
			//increase buf size
			// LOG("buf needed %zu\n", bufused + len);
			// LOG("bufsize is not big enough, current: %zu ", bufsize);
			// bufsize = bufsize * 2;
			// bufsize = ((bufused + len) / 256 + 1) * 256;
			if (i == arraylen - 1) {
				// last element
				// only (bufused + len) is really needed but give it a bit of headroom 
				// since its likely a similar command will be fired again (and it might
				// be slightly larger)
				bufsize = (1.2 * (bufused + len) / 256 + 1) * 256;
			} else {
				// estimate remaining space that is needed
				float avarage_arg_size = float(bufused + len) / i;
				// this is definitely needed
				bufsize = ((bufused + len +
					// estimated bytes needed for the remainder of the args
					(arraylen - 1 - i) * avarage_arg_size
					// plus some extra headroom
					* 1.2) / 256 + 1) * 256;
			}
			// LOG("increase it to %zu, ", bufsize);
			bufsize = ceil((float)bufsize / 64) * 64;  
			// LOG("but rounded up to multiple of 64: %zu\n", bufsize);
			char *new_buf = (char*)realloc(buf, bufsize);
			if (new_buf != buf) {
				buf = new_buf;;
				size_t bufused_ = 0;
				for(uint32_t j=0;j<i;j++) {
					argv[j] = buf + bufused_;
					bufused_ += argvlen[j];
				}
			}
			//continue from the same index again
			//i will be ++ed before next iteration
			i--;
			continue;
		}
		argv[i] = buf + bufused;
		memcpy(argv[i], *str, len);
		bufused += len;
		argvlen[i] = len;
		//LOG("added \"%.*s\" len: %zu\n", int(argvlen[i]), argv[i], argvlen[i]);
	}
	
	// LOG("total bufused %zu\n", bufused);
	//LOG("command buffer filled with: \"%.*s\"\n", int(bufused), buf);
	uint32_t callback_id = self->callback_id++;
	self->callbacksMap[callback_id].Reset(info[1].As<Napi::Function>());
	
	redisAsyncCommandArgv(
		self->c, 
		OnRedisResponse,
		(void*)(intptr_t)callback_id,
		arraylen,
		(const char**)argv,
		(const size_t*)argvlen
	);
	return env.Undefined();
}
