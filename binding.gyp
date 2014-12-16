{
    "targets": [
        {
            "dependencies": [
                "deps/hiredis.gyp:hiredis"
            ],
            "sources": [
                "src/redis-fast-driver.cc"
            ],
            "target_name": "redis-fast-driver",
            "defines": [
                "ENABLELOG=0"
            ]
        }
    ]
}
