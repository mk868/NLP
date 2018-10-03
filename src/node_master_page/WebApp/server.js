'use strict';
var config = require('./config.json');
var express = require('express');


var app = express();
var port = config.port;


var db = require("./db");

db.init(config.connectionString);

var devices = require("./devices");
devices.init(db);


var uow = {
    devices: devices,
    config: config,
    db: db
}

var devicesScheduler = require("./devicesScheduler");
devicesScheduler.init(uow);

uow.devicesScheduler = devicesScheduler;


require("./routesWSdevices")(app, uow);
require("./routesPages")(app, uow);
require("./routesWebApi")(app, uow);
app.use(express.static('public'));

app.listen(port, function () {
    console.log('http app listening on port ' + port + '!');
});

