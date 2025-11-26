this._xhrWrapper = function(url, type, callback) {
    var xhr = new XMLHttpRequest();
    xhr.onload = function () {
        callback(xhr);
    };
    xhr.open(type, url);
    xhr.send();
};

this.sendToPebble = function(season, overviewJson) { 
    if (!overviewJson.data || Object.keys(overviewJson.data).length == 0) { 
        Pebble.sendAppMessage({
            "Error": "Season data not available"
        })
    }
    let today = (new Date()).toISOString().split('T')[0];
    let upcomingEvents = [];
    let previousEvents = [];
    for (let key of Object.keys(overviewJson.data)) { 
        let model = overviewJson.data[key];
        let schedule;
        if (model.schedule == null || model.schedule.length == 0) { 
            schedule = [];
        } else {
            schedule = model.schedule.map(x => {
                return { 
                    "date": x.date,
                    "time": x.time,
                    "label": x.label
                }
            })
        }

        if (model.date >= today) { 
            upcomingEvents.push({
                "season": model.season,
                "round": model.round,
                "name": model.name,
                "circuit": model.circuit.name,
                "country": model.circuit.country,
                "date": model.date,
                "time": model.time,
                "events": schedule
            })
        } else { 
            previousEvents.push({
                "season": model.season,
                "round": model.round,
                "name": model.name,
                "circuit": model.circuit.name,
                "country": model.circuit.country,
                "date": model.date,
                "time": model.time,
                "events": schedule
            })
        }
    }

    Pebble.sendAppMessage({
        "Calendar": {
            "Season": season,
            "UpcomingEvents": upcomingEvents,
            "PreviousEvents": previousEvents
        }
    })
}


Pebble.addEventListener("ready", function(e) {
    console.log("Hello world! - Sent from your javascript application.");
});

Pebble.addEventListener("appmessage", function(dict) {
    console.log("App Message");
    console.log('Got message: ' + JSON.stringify(dict));
});