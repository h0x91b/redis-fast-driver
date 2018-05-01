#ifndef BUILDING_NODE_EXTENSION
	#define BUILDING_NODE_EXTENSION
#endif
#include <math.h>
#include <node.h>
#include <node_version.h>
#include <v8.h>
#include "redis-fast-driver.h"

using namespace v8;

void init(Handle<Object> exports) {
	RedisConnector::Init(exports);
}

NODE_MODULE(redis_fast_driver, init)

Nan::Persistent<Function> RedisConnector::constructor;

RedisConnector::RedisConnector() {
	LOG("%s\n", __PRETTY_FUNCTION__);
	callback_id = 1;
}

RedisConnector::~RedisConnector() {
	LOG("%s\n", __PRETTY_FUNCTION__);
}

void RedisConnector::Init(Handle<Object> exports) {
	Nan::HandleScope scope;
	Local<FunctionTemplate> tpl = Nan::New<FunctionTemplate>(New);
	tpl->SetClassName(Nan::New<String>("RedisConnector").ToLocalChecked());
	tpl->InstanceTemplate()->SetInternalFieldCount(1);
	// Prototype
	Nan::SetPrototypeTemplate(tpl, "connect", Nan::New<FunctionTemplate>(Connect));
	Nan::SetPrototypeTemplate(tpl, "disconnect", Nan::New<FunctionTemplate>(Disconnect));
	Nan::SetPrototypeTemplate(tpl, "redisCmd", Nan::New<FunctionTemplate>(RedisCmd));
	constructor.Reset(tpl->GetFunction());
	exports->Set(Nan::New("RedisConnector").ToLocalChecked(), tpl->GetFunction());
}

NAN_METHOD(RedisConnector::New) {
	Nan::HandleScope scope;

	if (info.IsConstructCall()) {
		// Invoked as constructor: `new RedisConnector(...)`
		RedisConnector* obj = new RedisConnector();
		obj->Wrap(info.This());
		info.GetReturnValue().Set(info.This());
	} else {
		// Invoked as plain function `RedisConnector(...)`, turn into construct call.
		Nan::ThrowError("This function must be called as a constructor (e.g. new RedisConnector())");
	}
}

void RedisConnector::ConnectCallback(const redisAsyncContext *c, int status) {
	LOG("%s\n", __PRETTY_FUNCTION__);
	Nan::HandleScope scope;
	RedisConnector *self = (RedisConnector*)c->data;
	if (status != REDIS_OK) {
		LOG("%s !REDIS_OK\n", __PRETTY_FUNCTION__);
		self->is_connected = false;
		Local<Value> argv[1] = {
			Nan::New<String>(c->errstr).ToLocalChecked()
		};
		Nan::New(self->connectCb)->Call(Nan::GetCurrentContext()->Global(), 1, argv);
		return;
	}
	self->is_connected = true;
	Local<Value> argv[1] = {
		Nan::Null()
	};
	Nan::New(self->connectCb)->Call(Nan::GetCurrentContext()->Global(), 1, argv);
}

void RedisConnector::DisconnectCallback(const redisAsyncContext *c, int status) {
	LOG("%s\n", __PRETTY_FUNCTION__);
	Nan::HandleScope scope;
	RedisConnector *self = (RedisConnector*)c->data;
	self->is_connected = false;
	if (status != REDIS_OK) {
		Local<Value> argv[1] = {
			Nan::New<String>(c->errstr).ToLocalChecked()
		};
		Nan::New(self->disconnectCb)->Call(Nan::GetCurrentContext()->Global(), 1, argv);
		return;
	}
	Local<Value> argv[1] = {
		Nan::Null()
	};
	Nan::New(self->disconnectCb)->Call(Nan::GetCurrentContext()->Global(), 1, argv);
}

NAN_METHOD(RedisConnector::Disconnect) {
	LOG("%s\n", __PRETTY_FUNCTION__);
	Nan::HandleScope scope;
	RedisConnector* self = ObjectWrap::Unwrap<RedisConnector>(info.This());
	if(self->c->replies.head!=NULL) {
		LOG("there is more callbacks in queue...\n");
	}
	if(self->is_connected) redisAsyncDisconnect(self->c);
	self->is_connected = false;
	self->c = NULL;
	info.GetReturnValue().Set(Nan::Undefined());
}

NAN_METHOD(RedisConnector::Connect) {
	LOG("%s\n", __PRETTY_FUNCTION__);
	Nan::HandleScope scope;
	if(info.Length() != 4) {
		Nan::ThrowError("Wrong arguments count");
		info.GetReturnValue().Set(Nan::Undefined());
	}
	RedisConnector* self = ObjectWrap::Unwrap<RedisConnector>(info.This());
	
	String::Utf8Value v8str(info[0]);
	const char *host = *v8str;
	unsigned short port = (unsigned short)info[1]->NumberValue();
	Local<Function> connectCb = Local<Function>::Cast(info[2]);
	self->connectCb.Reset(connectCb);
	Local<Function> disconnectCb = Local<Function>::Cast(info[3]);
	self->disconnectCb.Reset(disconnectCb);
	Local<Function> setImmediate = Local<Function>::Cast(Nan::GetCurrentContext()->Global()->Get(Nan::New("setImmediate").ToLocalChecked()));
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
		Local<Value> argv[1] = {
			Nan::New<String>(self->c->errstr).ToLocalChecked()
		};
		Nan::New(self->connectCb)->Call(Nan::GetCurrentContext()->Global(), 1, argv);
		info.GetReturnValue().Set(Nan::Undefined());
		return;
	}
	uv_loop_t* loop = uv_default_loop();
	self->c->data = (void*)self;
	redisLibuvAttach(self->c,loop);
	redisAsyncSetConnectCallback(self->c,ConnectCallback);
	redisAsyncSetDisconnectCallback(self->c,DisconnectCallback);
	
	info.GetReturnValue().Set(Nan::Undefined());
}

Local<Value> ParseResponse(redisReply *reply, size_t* size) {
	Nan::EscapableHandleScope scope;
	Local<Value> resp;
	Local<Array> arr = Nan::New<Array>();
	
	switch(reply->type) {
	case REDIS_REPLY_NIL:
		resp = Nan::Null();
		*size += sizeof(NULL);
		break;
	case REDIS_REPLY_INTEGER:
		resp = Nan::New<Number>(reply->integer);
		*size += sizeof(int);
		break;
	case REDIS_REPLY_STATUS:
	case REDIS_REPLY_STRING:
		resp = Nan::New<String>(reply->str, reply->len).ToLocalChecked();
		*size += reply->len;
		break;
	case REDIS_REPLY_ARRAY:
		for (size_t i=0; i<reply->elements; i++) {
			arr->Set(Nan::New<Number>(i), ParseResponse(reply->element[i], size));
		}
		resp = arr;
		break;
	default:
		printf("Redis rotocol error, unknown type %d\n", reply->type);
		Nan::ThrowError("Protocol error, unknown type");
		return Nan::Undefined();
	}
	
	return scope.Escape(resp);
}

void RedisConnector::OnRedisResponse(redisAsyncContext *c, void *r, void *privdata) {
	Nan::HandleScope scope;
	size_t totalSize = 0;
	//LOG("%s\n", __PRETTY_FUNCTION__);
	redisReply *reply = (redisReply*)r;
	uint32_t callback_id = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(privdata));
	if (reply == NULL) return;
	RedisConnector *self = (RedisConnector*)c->data;
	Local<Function> jsCallback = Nan::New(self->callbacksMap[callback_id]);
	Nan::Callback cb(jsCallback);
	Local<Function> setImmediate = Nan::New(self->setImmediate);
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
		Local<Value> argv[4] = {
			jsCallback,
			Nan::New(reply->str).ToLocalChecked(),
			Nan::Undefined(),
			Nan::New<Number>(totalSize)
		};
		setImmediate->Call(Nan::GetCurrentContext()->Global(), 4, argv);
		return;
	}
	Local<Value> resp = ParseResponse(reply, &totalSize);
	if( resp->IsUndefined() ) {
		Local<Value> argv[4] = {
			jsCallback,
			Nan::New<String>("Protocol error, can not parse answer from redis").ToLocalChecked(),
			Nan::Undefined(),
			Nan::New<Number>(totalSize)
		};
		setImmediate->Call(Nan::GetCurrentContext()->Global(), 4, argv);
		return;
	}
	
	Local<Value> argv[4] = {
		jsCallback,
		Nan::Null(),
		resp,
		Nan::New<Number>(totalSize)
	};
	setImmediate->Call(Nan::GetCurrentContext()->Global(), 4, argv);
	
	// Local<Value> argv[3] = {
	// 	Nan::Null(),
	// 	resp,
	// 	Nan::New<Number>(totalSize)
	// };
	// jsCallback->Call(Nan::GetCurrentContext()->Global(), 3, argv);
}

NAN_METHOD(RedisConnector::RedisCmd) {
	//LOG("%s\n", __PRETTY_FUNCTION__);
	static size_t bufsize = RFD_COMMAND_BUFFER_SIZE;
	static char* buf = (char*)malloc(bufsize);
	static size_t argvroom = 128;
	static size_t *argvlen = (size_t*)malloc(argvroom * sizeof(size_t*));
	static char **argv = (char**)malloc(argvroom * sizeof(char*));
	
	size_t bufused = 0;
	Nan::HandleScope scope;
	if(info.Length() != 2) {
		Nan::ThrowError("Wrong arguments count");
		info.GetReturnValue().Set(Nan::Undefined());
	}
	RedisConnector* self = ObjectWrap::Unwrap<RedisConnector>(info.This());
	
	Local<Array> array = Local<Array>::Cast(info[0]);
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
		String::Utf8Value str(array->Get(i));
		uint32_t len = str.length();
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
	Isolate* isolate = Isolate::GetCurrent();
	self->callbacksMap[callback_id].Reset(isolate, Local<Function>::Cast(info[1]));
	
	redisAsyncCommandArgv(
		self->c, 
		OnRedisResponse,
		(void*)(intptr_t)callback_id,
		arraylen,
		(const char**)argv,
		(const size_t*)argvlen
	);
	info.GetReturnValue().Set(Nan::Undefined());
}
