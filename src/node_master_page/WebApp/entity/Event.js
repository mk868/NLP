module.exports = function (sequelize, DataTypes) {
    var Repeating = require("./../Repeating.js")

    return sequelize.define('event', {
        id: {
            type: DataTypes.INTEGER,
            primaryKey: true,
            autoIncrement: true
        },
        start_date: DataTypes.DATE,
        end_date: DataTypes.DATE,
        text: {
            type: DataTypes.STRING
            //todo not null
        },
        repeating: DataTypes.ENUM(Repeating.getAllElements()),
        repeating_end_date: DataTypes.DATE
    }, {
            freezeTableName: true // Model tableName will be the same as the model name
        });
};