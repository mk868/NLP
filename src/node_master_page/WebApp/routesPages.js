

module.exports = function (app, uow) {
    var basicAuth = require('express-basic-auth');



    //user site
    app.use(basicAuth({
        users: uow.config.auth,
        challenge: true
    }));
    //private access

    app.set('view engine', 'ejs');
    var bodyParser = require('body-parser')
    app.use(bodyParser.json());       // to support JSON-encoded bodies
    app.use(bodyParser.urlencoded({     // to support URL-encoded bodies
        extended: true
    }));


    // index page 
    app.get('/', function (req, res) {
        res.render('pages/index');
    });



    // about page 
    app.get('/about', function (req, res) {
        res.render('pages/about');
    });

    // about page 
    app.get('/calendar', function (req, res) {
        res.render('pages/calendar');
    });

    // devices page 
    app.get('/devices', function (req, res) {

        uow.db.Device.findAll().then(function (devices) {//todo add where

            for (var k in devices) {
                devices[k].connected = uow.devices.isOnline(devices[k].id);
            }

            res.render('pages/devices', { devices: devices });

        });
    });

    // devices page 
    app.get('/device/:id', function (req, res) {
        var id = req.params.id;
        uow.db.Device.findById(id).then(function (device) {//todo add where
            var id = device.id;

            if (id == null) {
                //todo device not exist
                return;
            }

            device.connected = uow.devices.isOnline(id);

            res.render('pages/device', { device: device });

        });
    });

    app.post('/device/:id', function (req, res) {
        var id = req.params.id;
        uow.db.Device.findById(id).then(function (device) {
            var id = device.id;

            if (id == null) {
                //todo device not exist
                return;
            }

            if (req.body.type == 1) {
                if (req.body.enable == 1)
                    uow.devices.enable(id, null);
                else
                    uow.devices.disable(id);
            }

            if (req.body.type == 2) {
                var time = parseInt(req.body.sec) + parseInt(req.body.min) * 60;// todo add validation

                uow.devices.enable(id, time);
            }

            res.redirect(req.url);
        });
    });
}

