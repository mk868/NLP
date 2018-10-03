module.exports = {
    Daily: "Daily",
    WorkingDays: "WorkingDays",
    EveryWeek: "EveryWeek",
    EveryMonth: "EveryMonth",
    EveryYear: "EveryYear",


    getAllElements: function () {
        return [this.Daily, this.WorkingDays, this.EveryWeek, this.EveryMonth, this.EveryYear];
    },

    addDaysToTimeByRepeating: function (start, end, repeating) {
    switch (repeating) {
        case this.Daily:
            start.setDate(start.getDate() + 1);
            end.setDate(end.getDate() + 1);
            break;
        case this.WorkingDays:
            var add = 1;
            if (start.getDay() >= 5)
                add = 8 - start.getDay();

            start.setDate(start.getDate() + add);
            end.setDate(end.getDate() + add);
            break;
        case this.EveryWeek:
            start.setDate(start.getDate() + 7);
            end.setDate(end.getDate() + 7);
            break;
        case this.EveryMonth:
            start.setDate(start.getDate() + 30);
            end.setDate(end.getDate() + 30);
            break;
        case this.EveryYear:
            start.setDate(start.getDate() + 365);
            end.setDate(end.getDate() + 365);
            break;
    }
}
};