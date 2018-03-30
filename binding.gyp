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
            "include_dirs" : [
                "<!(node -e \"require('nan')\")"
            ],
            "defines": [
                "ENABLELOG=0"
            ]
        }
    ]
}
