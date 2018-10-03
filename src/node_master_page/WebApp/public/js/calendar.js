

//MODAL

function setModalEvent(event) {
    var alert1 = false;
    if (event.id) {//editing mode
        event = event.group;

        if (event.repeating) {
            alert1 = true;
        }
    }
    $('#editEventModalAlert1').toggle(alert1);
    $('#editEventModal').data("id", event.id || null);
    $('#editEventModalDelete').toggle(event.id ? true : false);

    $('#editEventModalTitle').val(event.title || '');

    //set dates
    $('#editEventModalStartDate').data("DateTimePicker").maxDate(event.end);
    $('#editEventModalEndDate').data("DateTimePicker").minDate(event.start);
    $("#editEventModalStartDate").data("DateTimePicker").date(event.start);
    $("#editEventModalEndDate").data("DateTimePicker").date(event.end);


    $('#editEventModalRepeating').val(event.repeating).trigger('change');


    $("#editEventModalRepeatingEndDateEnabled").prop('checked', event.repeating_end_date ? true : false).trigger('change');

    if (event.repeating_end_date) {
        $("#editEventModalRepeatingEndDate").data("DateTimePicker").date(moment(event.repeating_end_date));
    }
}


function getModalEvent() {
    var event = {};

    event.device_id = $('#devicesListbox').val();
    event.id = $('#editEventModal').data("id");
    event.title = $('#editEventModalTitle').val();
    event.start = $("#editEventModalStartDate").data("DateTimePicker").date();
    event.end = $("#editEventModalEndDate").data("DateTimePicker").date();
    event.repeating = $('#editEventModalRepeating').val() || null;
    var repeatingEndDateEnabled = $("#editEventModalRepeatingEndDateEnabled").is(':checked');
    if (event.repeating && repeatingEndDateEnabled) {
        event.repeating_end_date = $("#editEventModalRepeatingEndDate").data("DateTimePicker").date();
    }

    return event;
}


function showEditModal(event) {
    $('#editEventModal form')[0].reset();

    setModalEvent(event);    

    $('#editEventModal').modal('show');
}

$(document).ready(function () {
    $('#editEventModalStartDate, #editEventModalEndDate, #editEventModalRepeatingEndDate').datetimepicker({
        //defaultDate: new Date(),
        format: 'DD/MM/YYYY HH:mm:ss',
        sideBySide: true,
        useCurrent: false,
        locale: 'en-gb'
        //weekStart: 0
        //weekNumberCalculation: "ISO"
        //weekStart: 1
    });

    //end date > start date
    $("#editEventModalStartDate").on("dp.change", function (e) {
        $('#editEventModalEndDate').data("DateTimePicker").minDate(e.date);
    });
    $("#editEventModalEndDate").on("dp.change", function (e) {
        $('#editEventModalStartDate').data("DateTimePicker").maxDate(e.date);
        $('#editEventModalRepeatingEndDate').data("DateTimePicker").minDate(e.date);
    });

    //unselect after close
    $('#editEventModal').on('hidden.bs.modal', function () {
        $('#calendar').fullCalendar('unselect');
    });

    //show repeating when selected in list
    $('#editEventModalRepeating').on('change', function () {
        $('#editEventModalRepeatingEndDateGroup').toggle(this.value ? true : false);
    })

    //repeating date field on checkbox
    $('#editEventModalRepeatingEndDateEnabled').change(function () {
        $('#editEventModalRepeatingEndDate').attr('disabled', !(this.checked))
    });


    //delete existing event
    $('#editEventModalDelete').click(function () {
        var id = $('#editEventModal').data("id");

        if (!id)
            return;

        if (!confirm("Are you sure you want to delete the event?"))
            return;

        var event = {};

        event.id = id;
        event.device_id = $('#devicesListbox').val();

        $('#calendar').fullCalendar('renderEvent', event, false); // stick? = true

        $.ajax({
            url: "/api/events",
            type: "DELETE",
            data: JSON.stringify({ event: event }),
            contentType: "application/json; charset=utf-8",
            dataType: "json",
            success: function (data) {
                $('#calendar').fullCalendar('refetchEvents');
            }
        });
    });

    //on save click
    $('#editEventModalSave').click(function () {
        var event = getModalEvent();
        
        $.ajax({
            url: "/api/events",
            type: "PUT",
            data: JSON.stringify({ event: event }),
            contentType: "application/json; charset=utf-8",
            dataType: "json",
            success: function (data) {
                $('#calendar').fullCalendar('refetchEvents');
            }
        });
    });



});



//modal end






