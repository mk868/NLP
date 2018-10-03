module.exports = {
    _uow: null,
    _jobs: {},
    _schedule: null,
    
    init: function (uow) {
        this._uow = uow;
        this._schedule = require('node-schedule');
    },

    updateForDevice: function (id) {
        var thisModule = this;
        var Repeating = require("./Repeating");

        var now = new Date();

        var nowEnabled = this._jobs[id] ? this._jobs[id].enabled : false;
        this._stopTask(id);

        this._uow.db.sequelize.query(`
        SELECT id, start_date, end_date, text, repeating, repeating_end_date
        FROM public.event
        WHERE
            "deviceId" = :device_id AND
            (
                (
                    repeating IS NULL AND
                    (
                        start_date > :date OR 
                        end_date > :date
                    )
                ) OR
                (
                    repeating IS NOT NULL AND
                    (
                        repeating_end_date IS NULL OR
                        repeating_end_date >= :date
                    )
                )
            )`,
            { replacements: { date: now, device_id: id }, type: this._uow.db.sequelize.QueryTypes.SELECT })
            .then(function (events) {

                if (events.length == 0) {
                    return;
                }

                for (var k = 0; k < events.length; k++) {
                    var event = events[k];

                    event.closestDate = {
                        start_date: event.start_date,
                        end_date: event.end_date
                    };
                    if (event.closestDate.start_date > now) {
                        event.closestDate.delta = event.closestDate.start_date.getTime() - now.getTime();
                    } else {
                        event.closestDate.delta = event.closestDate.end_date.getTime() - now.getTime();
                    }



                    if (!event.repeating)
                        continue;

                    var newStartDate = new Date(event.start_date);
                    var newEndDate = new Date(event.end_date);

                    while (newStartDate < now && newEndDate < now && (!event.repeating_end_date || newEndDate < event.repeating_end_date)) {
                        Repeating.addDaysToTimeByRepeating(newStartDate, newEndDate, event.repeating);

                        if (newEndDate > now) {
                            event.closestDate = {
                                start_date: newStartDate,
                                end_date: newEndDate
                            };
                            if (event.closestDate.start_date > now) {
                                event.closestDate.delta = event.closestDate.start_date.getTime() - now.getTime();
                            } else {
                                event.closestDate.delta = event.closestDate.end_date.getTime() - now.getTime();
                            }
                        }
                    }
                }


                var closestIndex = 0;
                for (var i = 1; i < events.length; i++) {
                    var closestEvent = events[closestIndex];
                    var event = events[i];

                    if (event.closestDate.delta < closestEvent.closestDate.delta)
                        closestIndex = i;
                }

                var event = events[closestIndex];
                var enable = event.closestDate.start_date > now;
                var date = enable ? event.closestDate.start_date : event.closestDate.end_date;
                thisModule._startTask(id, date, enable);

                if (!enable) {//device should be enabled now
                    thisModule._enable(id);
                }
                if (enable && nowEnabled) {
                    thisModule._disable(id);
                }
            });
    },

    _startTask: function (id, date, enable) {
        var thisModule = this;
        console.log("start task " + date.toISOString() + " device[" + id + "] mode: " + enable);
        this._jobs[id] = this._schedule.scheduleJob(date, function (id, enable) {
            console.log('change device state[' + id + '] mode: ' + enable);
            if (enable) {
                thisModule._enable(id);
            } else {
                thisModule._disable(id);
            }

            thisModule.updateForDevice(id);
        }.bind(null, id, enable));
        this._jobs[id].enabled = !enable;
    },

    _stopTask: function (id) {
        if (this._jobs[id]) {
            this._jobs[id].cancel();
            this._jobs[id] = null;
        }
    },

    _enable: function (id) {
        this._uow.devices.enable(id);
    },

    _disable: function (id) {
        this._uow.devices.disable(id);
    }

}