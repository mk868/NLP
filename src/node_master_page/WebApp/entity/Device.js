module.exports = function (sequelize, DataTypes) {
    return sequelize.define('device', {
        id: {
            type: DataTypes.INTEGER,
            primaryKey: true,
            autoIncrement: true
        },
        device_id: DataTypes.INTEGER,
        name: DataTypes.STRING,
        color: DataTypes.STRING
    }, {
            freezeTableName: true
        });
};