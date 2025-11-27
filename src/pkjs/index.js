var seasons = {};
var currentSeason = new Date().getFullYear();

var REQUEST_TYPE = {
    CALENDAR: 0,
    DRIVERS: 1,
    TEAMS: 2
};

function fetchSeasonData(season, callback) {
    if (seasons[season]) {
        callback(seasons[season]);
        return;
    }

    var overviewUrl = 'https://sand.flashback.pages.dev/overview/' + season + '.json';
    var standingsUrl = 'https://sand.flashback.pages.dev/standings/' + season + '.json';

    Promise.all([
        fetch(overviewUrl).then(function (response) { return response.json(); }),
        fetch(standingsUrl).then(function (response) { return response.json(); })
    ]).then(function (values) {
        var overview = values[0];
        var standings = values[1];

        seasons[season] = {
            overview: overview,
            standings: standings
        };

        console.log('Fetched data for season ' + season);
        callback(seasons[season]);
    }).catch(function (err) {
        console.log('Error fetching data: ' + err);
    });
}

function sendAppMessage(dict) {
    Pebble.sendAppMessage(dict, function () {
        console.log('Message sent successfully: ' + JSON.stringify(dict));
    }, function (e) {
        console.log('Message failed: ' + JSON.stringify(e));
    });
}

function sendList(data, type) {
    // Send count first
    var countDict = {};
    countDict[10000] = type; // REQUEST_TYPE
    countDict[10002] = data.length; // DATA_COUNT

    console.log('Sending count dict: ' + JSON.stringify(countDict));
    Pebble.sendAppMessage(countDict, function () {
        console.log('Count sent successfully');
        // Send items one by one
        sendNextItem(data, type, 0);
    }, function (e) {
        console.log('Failed to send count: ' + JSON.stringify(e));
    });
}

function sendNextItem(data, type, index) {
    if (index >= data.length) {
        console.log('Finished sending list');
        return;
    }

    var item = data[index];
    var dict = {};
    dict[10000] = type; // REQUEST_TYPE
    dict[10001] = index; // DATA_INDEX
    dict[10003] = item.title; // DATA_TITLE
    dict[10004] = item.subtitle; // DATA_SUBTITLE
    dict[10005] = item.extra; // DATA_EXTRA

    Pebble.sendAppMessage(dict, function () {
        sendNextItem(data, type, index + 1);
    }, function (e) {
        console.log('Failed to send item ' + index + ': ' + JSON.stringify(e));
    });
}

Pebble.addEventListener('ready', function () {
    console.log('PebbleKit JS ready!');
    console.log('Registering appmessage listener...');
    fetchSeasonData(currentSeason, function () {
        console.log('Initial data loaded');
    });
});

Pebble.addEventListener('appmessage', function (e) {
    console.log('!!! Received AppMessage event !!!');
    console.log('Event payload keys: ' + JSON.stringify(Object.keys(e.payload)));
    console.log('Full payload: ' + JSON.stringify(e.payload));

    var dict = e.payload;
    var requestType = dict.REQUEST_TYPE;

    console.log('Request type value: ' + requestType);

    fetchSeasonData(currentSeason, function (data) {
        console.log('Processing request type: ' + requestType);
        console.log('Data structure: ' + JSON.stringify(Object.keys(data)));

        if (requestType === REQUEST_TYPE.CALENDAR) {
            // Process calendar data
            // API returns object with keys like "s2025r1", "s2025r2" inside data.overview.data
            console.log('Overview keys: ' + JSON.stringify(Object.keys(data.overview)));
            var racesObj = data.overview.data;
            console.log('Races object type: ' + typeof racesObj);
            console.log('Races keys: ' + (racesObj ? JSON.stringify(Object.keys(racesObj).slice(0, 3)) : 'null'));

            var racesArray = [];
            for (var key in racesObj) {
                if (racesObj.hasOwnProperty(key)) {
                    racesArray.push(racesObj[key]);
                }
            }
            console.log('Found ' + racesArray.length + ' races');

            // Sort by round
            racesArray.sort(function (a, b) {
                return a.round - b.round;
            });

            var races = racesArray.map(function (race) {
                var details = race.name + '\n' + race.circuit.city + '\n\n';
                if (race.schedule) {
                    // Schedule is an array in the new API
                    race.schedule.forEach(function (session) {
                        var time = session.date + ' ' + session.time;
                        details += session.label.toUpperCase() + ':\n' + time.replace('T', ' ').replace('Z', '') + '\n';
                    });
                }
                return {
                    title: race.name,
                    subtitle: race.circuit.city + ', ' + race.circuit.country,
                    extra: details
                };
            });
            console.log('Sending ' + races.length + ' races');
            sendList(races, REQUEST_TYPE.CALENDAR);

        } else if (requestType === REQUEST_TYPE.DRIVERS) {
            // Driver standings is an object in data.standings.data.driverStandings
            var driversObj = data.standings.data.driverStandings;
            var driversArray = [];
            for (var dKey in driversObj) {
                if (driversObj.hasOwnProperty(dKey)) {
                    var dEntry = driversObj[dKey];
                    // We need the driver name, which is in data.standings.data.drivers[dKey]
                    // But the standing entry has driverId which matches the key
                    var driverInfo = data.standings.data.drivers[dEntry.driverId];
                    dEntry.name = driverInfo.firstName + ' ' + driverInfo.lastName;
                    dEntry.code = driverInfo.code;
                    driversArray.push(dEntry);
                }
            }
            driversArray.sort(function (a, b) {
                return a.position - b.position;
            });

            var drivers = driversArray.map(function (driver) {
                return {
                    title: driver.position + '. ' + driver.code,
                    subtitle: driver.points + ' pts',
                    extra: driver.name
                };
            });
            sendList(drivers, REQUEST_TYPE.DRIVERS);

        } else if (requestType === REQUEST_TYPE.TEAMS) {
            // Constructor standings is an object in data.standings.data.constructorStandings
            var teamsObj = data.standings.data.constructorStandings;
            var teamsArray = [];
            for (var tKey in teamsObj) {
                if (teamsObj.hasOwnProperty(tKey)) {
                    var tEntry = teamsObj[tKey];
                    var teamInfo = data.standings.data.constructors[tEntry.constructorId];
                    tEntry.name = teamInfo.name;
                    teamsArray.push(tEntry);
                }
            }
            teamsArray.sort(function (a, b) {
                return a.position - b.position;
            });

            var teams = teamsArray.map(function (team) {
                return {
                    title: team.position + '. ' + team.name,
                    subtitle: team.points + ' pts',
                    extra: ''
                };
            });
            sendList(teams, REQUEST_TYPE.TEAMS);
        }
    });
});
