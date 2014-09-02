#define BUILDING_NODE_EXTENSION
#include <node.h>
#include <node_version.h>
#include <v8.h>
#include "redis-cluster.h"

using namespace v8;

void init(Handle<Object> exports) {
	RedisConnector::Init(exports);
}

NODE_MODULE(redis_cluster, init)

Persistent<Function> RedisConnector::constructor;

RedisConnector::RedisConnector(double value) : value_(value) {
	callbacks = Persistent<Object>::New(Object::New());
	callback_id = 1;
}

RedisConnector::~RedisConnector() {
}

Handle<Value> RedisConnector::PlusOne(const Arguments& args) {
	HandleScope scope;

	RedisConnector* obj = ObjectWrap::Unwrap<RedisConnector>(args.This());
	obj->value_ += 1;

	return scope.Close(Number::New(obj->value_));
}

void RedisConnector::Init(Handle<Object> exports) {
	// Prepare constructor template
	Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
	tpl->SetClassName(String::NewSymbol("RedisConnector"));
	tpl->InstanceTemplate()->SetInternalFieldCount(1);
	// Prototype
	tpl->PrototypeTemplate()->Set(String::NewSymbol("plusOne"), FunctionTemplate::New(PlusOne)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewSymbol("connect"), FunctionTemplate::New(Connect)->GetFunction());
	tpl->PrototypeTemplate()->Set(String::NewSymbol("redisCmd"), FunctionTemplate::New(RedisCmd)->GetFunction());
	constructor = Persistent<Function>::New(tpl->GetFunction());
	exports->Set(String::NewSymbol("RedisConnector"), constructor);
}

Handle<Value> RedisConnector::New(const Arguments& args) {
	HandleScope scope;

	if (args.IsConstructCall()) {
		// Invoked as constructor: `new RedisConnector(...)`
		double value = args[0]->IsUndefined() ? 0 : args[0]->NumberValue();
		RedisConnector* obj = new RedisConnector(value);
		obj->Wrap(args.This());
		args.This()->Set(String::NewSymbol("hh"), obj->callbacks);
		return args.This();
	} else {
		// Invoked as plain function `RedisConnector(...)`, turn into construct call.
		const int argc = 1;
		Local<Value> argv[argc] = { args[0] };
		return scope.Close(constructor->NewInstance(argc, argv));
	}
}

void RedisConnector::connectCallback(const redisAsyncContext *c, int status) {
	RedisConnector *self = (RedisConnector*)c->data;
	if (status != REDIS_OK) {
		Local<Value> argv[1] = {
			Local<Value>::New(String::New(c->errstr))
		};
		self->connectCb->Call(Context::GetCurrent()->Global(), 1, argv);
		return;
	}
	Local<Value> argv[1] = {
		Local<Value>::New(Null())
	};
	self->connectCb->Call(Context::GetCurrent()->Global(), 1, argv);
}

Handle<Value> RedisConnector::Connect(const Arguments& args) {
	printf("%s\n", __PRETTY_FUNCTION__);
	HandleScope scope;
	if(args.Length() != 3) {
		ThrowException(Exception::TypeError(String::New("Wrong arguments count")));
		return scope.Close(Undefined());
	}
	RedisConnector* self = ObjectWrap::Unwrap<RedisConnector>(args.This());
	
	String::Utf8Value v8str(args[0]);
	const char *host = *v8str;
	unsigned short port = (unsigned short)args[1]->NumberValue();
	self->connectCb = Persistent<Function>::New(Local<Function>::Cast(args[2]));
	printf("connect to %s:%d\n", host, port);
	
	self->c = redisAsyncConnect(host, port);
	if (self->c->err) {
		printf("Error: %s\n", self->c->errstr);
		// handle error
		ThrowException(Exception::TypeError(String::New(self->c->errstr)));
		return scope.Close(Undefined());
	}
	uv_loop_t* loop = uv_default_loop();
	self->c->data = (void*)self;
	redisLibuvAttach(self->c,loop);
	redisAsyncSetConnectCallback(self->c,connectCallback);
	
	return scope.Close(Undefined());
}

void RedisConnector::getCallback(redisAsyncContext *c, void *r, void *privdata) {
	HandleScope scope;
	//printf("%s\n", __PRETTY_FUNCTION__);
	redisReply *reply = (redisReply*)r;
	uint32_t callback_id = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(privdata));
	if (reply == NULL) return;
	RedisConnector *self = (RedisConnector*)c->data;
	Local<Function> cb = Local<Function>::Cast(self->callbacks->Get(Number::New(callback_id)));
	self->callbacks->Delete(Number::New(callback_id)->ToString());
	if (reply->type == REDIS_REPLY_ERROR) {
		printf("[%d] redis error: %s\n", callback_id, reply->str);
		Local<Value> argv[1] = {
			Local<Value>::New(String::New(reply->str))
		};
		cb->Call(Context::GetCurrent()->Global(), 1, argv);
		return;
	}
	Local<Value> resp;
	
	switch(reply->type) {
	case REDIS_REPLY_INTEGER:
		resp = Local<Value>::New(Number::New(reply->integer));
		break;
	case REDIS_REPLY_STATUS:
	case REDIS_REPLY_STRING:
		resp = Local<Value>::New(String::New(reply->str));
		break;
	default:
		printf("[%d] protocol error type %d\n", callback_id, reply->type);
		Local<Value> argv[1] = {
			Local<Value>::New(String::New("Protocol error, unknown type"))
		};
		cb->Call(Context::GetCurrent()->Global(), 1, argv);
		return;
	}
	
	Local<Value> argv[2] = {
		Local<Value>::New(Null()),
		resp
	};
	cb->Call(Context::GetCurrent()->Global(), 2, argv);
}

Handle<Value> RedisConnector::RedisCmd(const Arguments& args) {
	//printf("%s\n", __PRETTY_FUNCTION__);
	HandleScope scope;
	if(args.Length() != 2) {
		ThrowException(Exception::TypeError(String::New("Wrong arguments count")));
		return scope.Close(Undefined());
	}
	RedisConnector* self = ObjectWrap::Unwrap<RedisConnector>(args.This());
	
	Local<Array> array = Local<Array>::Cast(args[0]);
	Local<Function> cb = Local<Function>::Cast(args[1]);
	//Persistent<Function> cb = Persistent<Function>::New(Local<Function>::Cast(args[1]));
	char **argv = (char**)malloc(array->Length()*sizeof(char*));
	size_t *argvlen = (size_t*)malloc(array->Length()*sizeof(size_t*));
	uint32_t callback_id = self->callback_id++;
	self->callbacks->Set(Number::New(callback_id), cb);
	
	for(uint32_t i=0;i<array->Length();i++) {
		String::Utf8Value str(array->Get(i));
		argv[i] = (char*)malloc(str.length());
		memcpy(argv[i], *str, str.length());
		argvlen[i] = str.length();
		//printf("add \"%s\" len: %d\n", argv[i], argvlen[i]);
	}
	redisAsyncCommandArgv(self->c, getCallback, (void*)callback_id, array->Length(), (const char**)argv, (const size_t*)argvlen);
	for(uint32_t i=0;i<array->Length();i++) {
		free(argv[i]);
	}
	free(argv);
	free(argvlen);
	
	return scope.Close(Undefined());
}