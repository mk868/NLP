module.exports = {
    sequelize: null,
    Event: null,
    Device: null,

    init: function (connectionString) {
        var Sequelize = require('sequelize');

        var sequelize = new Sequelize(connectionString);
        
        var Event = require("./entity/Event")(sequelize, Sequelize);
        var Device = require("./entity/Device")(sequelize, Sequelize);
        

        Device.hasMany(Event);
        Event.belongsTo(Device);


        sequelize.sync();

        this.sequelize = sequelize;
        this.Event = Event;
        this.Device = Device;

    }
};