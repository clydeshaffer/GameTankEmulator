const { createServer } = require("http");
const fs = require('fs');
const path = require('path');

function mimetype(url) {
    switch(path.extname(url)) {
        case ".wasm":
            return "application/wasm";
        case ".json":
            return "application/json";
        case ".html":
            return "text/html";
        case ".js":
            return "text/javascript";
        case ".png":
            return "image/png";
    }
}

const PORT = process.env.PORT || 8080;

const server = createServer();

server.on("request", (request, response) => {
    if(request.method == "GET") {
        var url = request.url;
        if(url == "/") url = "/index.html";
        console.log(url);
        fs.readFile("./build" + url, {
            encoding : null
        }, (err, data) => {
            if(err) {
                response.statusCode = 404;
                response.end(err.message);
            } else {
                response.statusCode = 200;
                response.setHeader("Content-Type", mimetype(url));
                response.write(data);
                response.end();
            }
        })
    } else response.end("Hello, world!");
});

server.listen(PORT, () => {
  console.log(`starting server at port ${PORT}`);
});