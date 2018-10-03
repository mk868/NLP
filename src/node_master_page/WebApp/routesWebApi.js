module.exports = function (app, uow) {

    app.get("/api/events", function (req, res) {
        var Repeating = require("./Repeating");

        var start = req.query.start;
        var end = req.query.end;

        var calendarStart = new Date(start);
        var calendarEnd = new Date(end);

        uow.db.sequelize.query(`
        SELECT id, start_date, end_date, text, repeating, repeating_end_date, "deviceId" as device_id
        FROM public.event
        WHERE
        (
            repeating IS NULL AND
            start_date < :end AND
            end_date > :start
        ) OR (
            repeating IS NOT NULL AND
            start_date < :end AND
            (
                repeating_end_date IS NULL OR
                repeating_end_date >= :start
            )
        )`,
            { replacements: { end: calendarEnd, start: calendarStart }, type: uow.db.sequelize.QueryTypes.SELECT })
            .then(function (events) {


                for (var k = 0; k < events.length; k++) {
                    var event = events[k];

                    event.portions = [];

                    event.portions.push({
                        start_date: event.start_date,
                        end_date: event.end_date
                    });


                    if (!event.repeating)
                        continue;

                    var newStartDate = new Date(event.start_date);
                    var newEndDate = new Date(event.end_date);

                    while (newStartDate < calendarEnd && (!event.repeating_end_date || newEndDate < event.repeating_end_date)) {
                        Repeating.addDaysToTimeByRepeating(newStartDate, newEndDate, event.repeating);

                        if (newEndDate > calendarStart)
                            event.portions.push({
                                start_date: new Date(newStartDate),
                                end_date: new Date(newEndDate)
                            });
                    }


                }

                res.json(events);
            });
    });


    app.put("/api/events", function (req, res) {
        var event = req.body.event;

        var device_id = event.device_id;
        var id = event.id || null;

        if (!device_id) {
            res.json({
                success: false
            });
            return;
        }


        var entry = {
            start_date: event.start,
            end_date: event.end,
            text: event.title,
            repeating: event.repeating,
            repeating_end_date: event.repeating ? event.repeating_end_date : null
        };

        if (id) {

            uow.db.Event.update(
                entry,
                {
                    where: {
                        id: id,
                        deviceId: device_id
                    }
                }).then(event => {
                    uow.devicesScheduler.updateForDevice(device_id);
                });
        } else {
            entry.deviceId = device_id;
            uow.db.Event.create(entry).then(event => {
                uow.devicesScheduler.updateForDevice(device_id);
            });
        }
        
        res.json({
            success: true
        });
    });


    app.delete("/api/events", function (req, res) {
        var event = req.body.event;

        var id = event.id;
        var device_id = event.device_id;

        uow.db.Event.destroy({
            where: {
                id: id,
                deviceId: device_id
            }
        }).then(() => {
            uow.devicesScheduler.updateForDevice(device_id);
            });

        res.json({ success: true });
    });


    app.get("/api/devices", function (req, res) {
        uow.db.sequelize.query(`
        SELECT id, color, name
        FROM public.device
        `,
            { type: uow.db.sequelize.QueryTypes.SELECT })
            .then(function (devices) {

                res.json(devices);
            });
    });
}

