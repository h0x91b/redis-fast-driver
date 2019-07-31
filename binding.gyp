{
    "targets": [
        {
            "dependencies": [
                "<!(node -p 'require(\"node-addon-api\").gyp')",
                "deps/hiredis.gyp:hiredis",
            ],
            "sources": [
                "src/redis-fast-driver.cc"
            ],
            "conditions": [
                ['OS=="mac"', {
                    'cflags+': ['-fvisibility=hidden'],
                    'xcode_settings': {
                      'GCC_SYMBOLS_PRIVATE_EXTERN': 'YES', # -fvisibility=hidden
                    }
                }]
            ],
            "target_name": "redis_fast_driver",
            "cflags!": [ "-fno-exceptions" ],
            "cflags_cc!": [ "-fno-exceptions" ],
            "xcode_settings": {
                "GCC_ENABLE_CPP_EXCEPTIONS": "YES",
                "CLANG_CXX_LIBRARY": "libc++",
                "MACOSX_DEPLOYMENT_TARGET": "10.7",
            },
            "msvs_settings": {
                "VCCLCompilerTool": { "ExceptionHandling": 1 },
            },
            "include_dirs" : [
                "<!@(node -p \"require('node-addon-api').include\")"
            ],
            "defines": [
                "ENABLELOG=1"
            ]
        }
    ]
}
