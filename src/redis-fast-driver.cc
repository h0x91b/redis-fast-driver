#ifndef BUILDING_NODE_EXTENSION
	#define BUILDING_NODE_EXTENSION
#endif
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

RedisConnector::RedisConnector(double value) : value_(value) {
	LOG("%s\n", __PRETTY_FUNCTION__);
	callbacks.Reset(Nan::New<Object>());
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
		double value = info[0]->IsUndefined() ? 0 : info[0]->NumberValue();
		RedisConnector* obj = new RedisConnector(value);
		obj->Wrap(info.This());
		info.This()->Set(Nan::New<String>("callbacks").ToLocalChecked(), Nan::New(obj->callbacks));
		info.GetReturnValue().Set(info.This());
	} else {
		// Invoked as plain function `RedisConnector(...)`, turn into construct call.
		const int argc = 1;
		Local<Value> argv[argc] = { info[0] };
		info.GetReturnValue().Set(Nan::New(constructor)->NewInstance(argc, argv));
	}
}

void RedisConnector::connectCallback(const redisAsyncContext *c, int status) {
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

void RedisConnector::disconnectCallback(const redisAsyncContext *c, int status) {
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
		Nan::ThrowTypeError("Wrong arguments count");
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
		fprintf(stderr, "RedisConnector::Connect Error: %s\n", self->c->errstr);
		fprintf(stderr, "RedisConnector::Connect Host: %s port: %d\n", host, port);
		// handle error
		Nan::ThrowError(self->c->errstr);
		info.GetReturnValue().Set(Nan::Undefined());
		return;
	}
	uv_loop_t* loop = uv_default_loop();
	self->c->data = (void*)self;
	redisLibuvAttach(self->c,loop);
	redisAsyncSetConnectCallback(self->c,connectCallback);
	redisAsyncSetDisconnectCallback(self->c,disconnectCallback);
	
	info.GetReturnValue().Set(Nan::Undefined());
}

Local<Value> parseResponse(redisReply *reply, size_t* size) {
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
			arr->Set(Nan::New<Number>(i), parseResponse(reply->element[i], size));
		}
		resp = arr;
		break;
	default:
		printf("Redis rotocol error, unknown type %d\n", reply->type);
		Nan::ThrowTypeError("Protocol error, unknown type");
		return Nan::Undefined();
	}
	
	return scope.Escape(resp);
}

void RedisConnector::getCallback(redisAsyncContext *c, void *r, void *privdata) {
	Nan::HandleScope scope;
	size_t totalSize = 0;
	//LOG("%s\n", __PRETTY_FUNCTION__);
	redisReply *reply = (redisReply*)r;
	uint32_t callback_id = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(privdata));
	if (reply == NULL) return;
	RedisConnector *self = (RedisConnector*)c->data;
	Local<Function> jsCallback = Local<Function>::Cast(Nan::New(self->callbacks)->Get(Nan::New(callback_id)));
	Nan::Callback cb(jsCallback);
	Local<Function> setImmediate = Nan::New(self->setImmediate);
	if(!(c->c.flags & REDIS_SUBSCRIBED || c->c.flags & REDIS_MONITORING)) {
		// LOG("delete, flags %i id %i\n", c->c.flags, callback_id);
		Nan::New<v8::Object>(self->callbacks)->Delete(Nan::New(callback_id)->ToString());
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
	Local<Value> resp = parseResponse(reply, &totalSize);
	// printf("Total size %lu\n", totalSize);
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
	static size_t bufsize = 4096;
	static char* buf = (char*)malloc(bufsize);
	static size_t argvroom = 128;
	static size_t *argvlen = (size_t*)malloc(argvroom * sizeof(size_t*));
	static char **argv = (char**)malloc(argvroom * sizeof(char*));
	
	size_t bufused = 0;
	Nan::HandleScope scope;
	if(info.Length() != 2) {
		Nan::ThrowTypeError("Wrong arguments count");
		info.GetReturnValue().Set(Nan::Undefined());
	}
	RedisConnector* self = ObjectWrap::Unwrap<RedisConnector>(info.This());
	
	Local<Array> array = Local<Array>::Cast(info[0]);
	Local<Function> cb = Local<Function>::Cast(info[1]);
	//Persistent<Function> cb = Persistent<Function>::New(Local<Function>::Cast(args[1]));
	size_t arraylen = array->Length();
	while(arraylen > argvroom) {
		// printf("double room for argv %zu\n", argvroom);
		argvroom *= 2;
		free(argvlen);
		free(argv);
		argvlen = (size_t*)malloc(argvroom * sizeof(size_t*));
		argv = (char**)malloc(argvroom * sizeof(char*));
	}
	uint32_t callback_id = self->callback_id++;
	Nan::New(self->callbacks)->Set(Nan::New<Number>(callback_id), cb);
	
processargs:
	for(uint32_t i=0;i<arraylen;i++) {
		String::Utf8Value str(array->Get(i));
		uint32_t len = str.length();
		while(bufused + len > bufsize) {
			//double buf size
			char *oldbuf = buf;
			bufsize *= 2;
			buf = (char*)malloc(bufsize);
			memcpy(buf, oldbuf, bufused);
			free(oldbuf);
			bufused = 0;
			goto processargs;
		}
		argv[i] = buf + bufused;
		memcpy(buf+bufused, *str, len);
		bufused += len;
		argvlen[i] = len;
		//LOG("add \"%s\" len: %d\n", argv[i], argvlen[i]);
	}
	redisAsyncCommandArgv(self->c, getCallback, (void*)(intptr_t)callback_id, arraylen, (const char**)argv, (const size_t*)argvlen);
	info.GetReturnValue().Set(Nan::Undefined());
}
