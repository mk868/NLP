module.exports = function (app, uow) {

    var expressWs = require('express-ws')(app);


    //public access
    app.ws(uow.config.wsPath, function (ws, req) {

        ws.isAlive = true;

        ws.on('message', function (msg) {
            try {
                var data = JSON.parse(msg);
            } catch (ex) {
                console.error("message error ", msg, ex);
                ws.close();
                return;
            }

            console.log(msg);

            var id = ws.id;

            if (!id && data.type == "hi") {//in first message it should set id
                if (!data.id) {
                    ws.close();
                    return;
                }

                var id = uow.devices.add(data.id, ws, function (success, id) {
                    if (!success) {
                        ws.close();
                        return;
                    }
                    ws.id = id;

                    uow.devicesScheduler.updateForDevice(id);
                });

                return;
            }

            if (!id) {
                return;
            }

            if (data.type == "state") {
                uow.devices.updateState(id, data.device, data.enabled);
            }


        });

        ws.on('pong', function () {
            this.isAlive = true;
        });

        ws.on('close', function () {
            if (!ws.id) {
                return;
            }
            console.log("bye: " + ws.id);
            uow.devices.remove(ws.id);
        });
    });

    //https://github.com/websockets/ws
    const interval = setInterval(function ping() {
        if (expressWs.getWss().clients.size == 0) {
            return;//no clients
        }

        expressWs.getWss().clients.forEach(function (ws) {
            if (ws.isAlive === false) {
                return ws.terminate();
            }

            ws.isAlive = false;
            ws.ping('', false, true);
        });
        console.log("ws ping");
    }, uow.config.wsPingTime);
}

