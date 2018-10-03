module.exports = {
    _db: null,
    _clients: {},

    init: function (db) {
        this._db = db;
    },

    isOnline: function (id) {
        return this._clients[id] ? true : false;
    },

    add: function (device_id, socket, cb) {
        var deviceInfo = this._db.Device.findOne({
            where: {
                device_id: device_id
            }
        }).then(device => {
            if (!device) {
                cb(false, undefined);
                return;
            }

            var id = device.id;

            this._clients[id] = {
                id: id,
                socket: socket,
                subDevices: {
                    "REL1": {
                    }
                }
            }

            cb(true, id);
        })
    },

    remove: function (id) {
        this._clients[id] = null;
    },


    enable: function (id, /*subDevice, */ time) {
        var subDevice = "REL1";

        if (!this._clients[id])
            return;

        if (!this._clients[id].subDevices[subDevice])
            return;

        this._clients[id].subDevices[subDevice].enabled = false;

        var msg = {
            type: "let",
            device: subDevice,
            enable: 1
        };

        if (time)
            msg.time = time;

        this._send(id, msg);
    },

    disable: function (id /*, subDevice*/) {
        var subDevice = "REL1";

        if (!this._clients[id])
            return;

        if (!this._clients[id].subDevices[subDevice])
            return;

        this._clients[id].subDevices[subDevice].enabled = false;

        var msg = {
            type: "let",
            device: subDevice,
            enable: 0
        };

        this._send(id, msg);
    },

    _send: function (id, message) {
        if (!this.isOnline(id)) {
            return;
        }

        this._clients[id].socket.send(JSON.stringify(message));
    },

    updateState: function (id, subDevice, enabled) {
        if (!this._clients[id])
            return;

        this._clients[id].subDevices[subDevice].enable = enabled;
    }

}