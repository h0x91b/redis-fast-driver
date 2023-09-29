var Redis = require(".");
var fs = require("fs").promises;
//var Redis = require('redis-fast-driver');

var r = new Redis({
  //host: '/tmp/redis.sock', //unix domain
  host: "127.0.0.1", //can be IP or hostname
  port: 6379,
  maxretries: 10, //reconnect retries, default -1 (infinity)
  // auth: '123',
  // db: 5
});

//happen only once
r.on("ready", async function () {
  console.log("redis ready");

  const bin = await fs.readFile("./binary-example.png");
  console.log("binary file size: %s", bin.length);
  r.rawCall(["set", "binary", bin.toString("binary")], (err, resp) => {
    console.log("SET via rawCall command returns err: %s, resp: %s", err, resp);
  });
  r.rawCall(["get", "binary"], async (err, resp) => {
    console.log(
      "GET via rawCall command returns err: %s, resp: %s",
      err,
      resp.length
    );
    const b = Buffer.from(resp, "binary");
    console.log("binary file size: %s", b.length);
    await fs.writeFile("./binary-example-out.png", b);
    r.end();
  });
});

//happen each time when reconnected
r.on("connected", function () {
  console.log("redis connected");
});

r.on("disconnected", function () {
  console.log("redis disconnected");
});

r.on("error", function (e) {
  console.log("redis error", e);
});
