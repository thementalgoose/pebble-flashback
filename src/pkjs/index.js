// PebbleKit JS - F1 Flashback Data Layer
// Handles API fetching, caching, and communication with watch

const CACHE_DURATION = 1000 * 60 * 60 * 12; // 12 hours
const BASE_URL = 'https://flashback.pages.dev';

// Clay configuration
var Clay = require('@rebble/clay');
var clayConfig = require('./config');
var clay = new Clay(clayConfig);

// Import auto-generated message keys
var messageKeys = require('message_keys');
console.log('Message keys loaded:', JSON.stringify(messageKeys));

// Request types (must match C code)
const REQUEST_TYPES = {
    GET_OVERVIEW: 1,
    GET_RACE_DETAILS: 2,
    GET_DRIVER_STANDINGS: 3,
    GET_TEAM_STANDINGS: 4
};

// Cache management
function getCacheKey(type, season) {
    return `f1_${type}_${season}`;
}

function getCachedData(type, season) {
    const key = getCacheKey(type, season);
    const cached = localStorage.getItem(key);

    if (!cached) {
        return null;
    }

    try {
        const data = JSON.parse(cached);
        const now = Date.now();

        // Check if cache is expired
        if (data.timestamp && (now - data.timestamp) < CACHE_DURATION) {
            console.log(`Cache hit for ${key}`);
            return data.content;
        } else {
            console.log(`Cache expired for ${key}`);
            localStorage.removeItem(key);
            return null;
        }
    } catch (e) {
        console.error('Error reading cache:', e);
        localStorage.removeItem(key);
        return null;
    }
}

function setCachedData(type, season, content) {
    const key = getCacheKey(type, season);
    const data = {
        timestamp: Date.now(),
        content: content
    };

    try {
        localStorage.setItem(key, JSON.stringify(data));
        console.log(`Cached data for ${key}`);
    } catch (e) {
        console.error('Error writing cache:', e);
    }
}

// Fetch data from API
function fetchOverview(season) {
    return new Promise((resolve, reject) => {
        // Check cache first
        const cached = getCachedData('overview', season);
        if (cached) {
            resolve(cached);
            return;
        }

        // Fetch from API using XMLHttpRequest (PebbleKit JS doesn't support fetch)
        const url = `${BASE_URL}/overview/${season}.json`;
        console.log(`Fetching overview from ${url}`);

        var xhr = new XMLHttpRequest();
        xhr.open('GET', url, true);
        xhr.onload = function() {
            if (xhr.status === 200) {
                try {
                    var data = JSON.parse(xhr.responseText);
                    console.log('Overview data received');
                    setCachedData('overview', season, data);
                    resolve(data);
                } catch (e) {
                    console.error('Error parsing JSON:', e);
                    reject(e);
                }
            } else {
                console.error('HTTP error:', xhr.status);
                reject(new Error('HTTP ' + xhr.status));
            }
        };
        xhr.onerror = function() {
            console.error('Network error');
            reject(new Error('Network error'));
        };
        xhr.send();
    });
}

function fetchStandings(season) {
    return new Promise((resolve, reject) => {
        // Check cache first
        const cached = getCachedData('standings', season);
        if (cached) {
            resolve(cached);
            return;
        }

        // Fetch from API using XMLHttpRequest (PebbleKit JS doesn't support fetch)
        const url = `${BASE_URL}/standings/${season}.json`;
        console.log(`Fetching standings from ${url}`);

        var xhr = new XMLHttpRequest();
        xhr.open('GET', url, true);
        xhr.onload = function() {
            if (xhr.status === 200) {
                try {
                    var data = JSON.parse(xhr.responseText);
                    console.log('Standings data received');
                    setCachedData('standings', season, data);
                    resolve(data);
                } catch (e) {
                    console.error('Error parsing JSON:', e);
                    reject(e);
                }
            } else {
                console.error('HTTP error:', xhr.status);
                reject(new Error('HTTP ' + xhr.status));
            }
        };
        xhr.onerror = function() {
            console.error('Network error');
            reject(new Error('Network error'));
        };
        xhr.send();
    });
}

// Process overview data and send races to watch
function sendRacesToWatch(overviewData) {
    if (!overviewData || !overviewData.data) {
        console.error('Invalid overview data');
        return;
    }

    const allRaces = Object.values(overviewData.data);
    const now = new Date();

    // Separate races into upcoming and past
    const upcomingRaces = allRaces.filter(race => new Date(race.date) >= now)
        .sort((a, b) => a.round - b.round); // Chronological order (earliest first)

    const pastRaces = allRaces.filter(race => new Date(race.date) < now)
        .sort((a, b) => b.round - a.round); // Reverse chronological (most recent first)

    console.log(`Formatting ${upcomingRaces.length} upcoming and ${pastRaces.length} past races`);

    // Build pipe-delimited strings for upcoming races
    const upcomingLines = upcomingRaces.map(race => {
        const location = `${race.circuit.city}, ${race.circuit.country}`;
        return `${race.round}|${race.name}|${location}`;
    });

    // Build pipe-delimited strings for past races
    const pastLines = pastRaces.map(race => {
        const location = `${race.circuit.city}, ${race.circuit.country}`;
        return `${race.round}|${race.name}|${location}`;
    });

    const upcomingText = upcomingLines.join('\n');
    const pastText = pastLines.join('\n');

    console.log('Sending upcoming races as single message');
    console.log('Upcoming text length:', upcomingText.length);

    // Send upcoming races
    Pebble.sendAppMessage({
        REQUEST_TYPE: REQUEST_TYPES.GET_OVERVIEW,
        DATA_INDEX: 0, // 0 = upcoming
        DATA_TITLE: upcomingText
    }, function () {
        console.log('Sent upcoming races successfully');
    }, function (e) {
        console.error('Failed to send upcoming races:', e);
        console.error('Error details:', JSON.stringify(e));
    });

    console.log('Sending past races as single message');
    console.log('Past text length:', pastText.length);

    // Send past races (with a small delay to ensure ordering)
    setTimeout(() => {
        Pebble.sendAppMessage({
            REQUEST_TYPE: REQUEST_TYPES.GET_OVERVIEW,
            DATA_INDEX: 1, // 1 = past
            DATA_TITLE: pastText
        }, function () {
            console.log('Sent past races successfully');
        }, function (e) {
            console.error('Failed to send past races:', e);
            console.error('Error details:', JSON.stringify(e));
        });
    }, 100);
}

// Abbreviate a full event label to its shorthand code
function abbreviateEvent(label) {
    if (!label) return '';
    if (/Free Practice 1|Practice 1/.test(label)) return 'FP1';
    if (/Free Practice 2|Practice 2/.test(label)) return 'FP2';
    if (/Free Practice 3|Practice 3/.test(label)) return 'FP3';
    if (/Sprint Qualifying|Sprint Shootout/.test(label)) return 'SQ';
    if (/Sprint/.test(label)) return 'SR';
    if (/Qualifying/.test(label)) return 'Q';
    if (/Race/.test(label)) return 'R';
    return label.substring(0, 3);
}

// Process race details and send events to watch
function sendRaceDetailsToWatch(overviewData, raceRound) {
    if (!overviewData || !overviewData.data) {
        console.error('Invalid overview data');
        return;
    }

    // Find the race with the matching round number
    const race = Object.values(overviewData.data).find(r => r.round === raceRound);

    if (!race) {
        console.error('Race not found for round:', raceRound);
        return;
    }

    const events = race.schedule || [];

    console.log(`Formatting ${events.length} events for ${race.name} (round ${raceRound})`);

    // Build pipe-delimited strings for all events
    const eventLines = events.map(event => {
        // Combine date and time into ISO format
        const dateTimeStr = event.date + 'T' + event.time;
        return `${abbreviateEvent(event.label)}|${dateTimeStr}`;
    });

    const eventsText = eventLines.join('\n');

    console.log('Sending race events as single message');
    console.log('Events text length:', eventsText.length);

    // Send as a single message with the formatted text
    Pebble.sendAppMessage({
        REQUEST_TYPE: REQUEST_TYPES.GET_RACE_DETAILS,
        DATA_TITLE: eventsText
    }, function () {
        console.log('Sent race events successfully');
    }, function (e) {
        console.error('Failed to send race events:', e);
        console.error('Error details:', JSON.stringify(e));
    });
}

// Process standings data and send driver standings to watch
function sendDriverStandingsToWatch(standingsData) {
    if (!standingsData || !standingsData.data) {
        console.error('Invalid standings data');
        return;
    }

    const driverStandings = standingsData.data.driverStandings;
    const drivers = standingsData.data.drivers;

    if (!driverStandings || !drivers) {
        console.error('Missing driver standings or drivers data');
        return;
    }

    // Convert driverStandings object to array and sort by position
    const standingsArray = Object.values(driverStandings).sort((a, b) => a.position - b.position);

    console.log(`Formatting ${standingsArray.length} driver standings as text`);

    // Build a formatted string with all driver standings
    const lines = [];
    standingsArray.forEach((standing) => {
        const driver = drivers[standing.driverId];
        if (!driver) {
            console.error('Driver not found:', standing.driverId);
            return;
        }

        const fullName = driver.firstName + ' ' + driver.lastName;
        const code = driver.code || standing.driverId.toUpperCase().substring(0, 3);

        // Format: "position|name|code|points pts"
        lines.push(`${standing.position}|${fullName}|${code}|${standing.points} pts`);
    });

    const formattedText = lines.join('\n');

    console.log('Sending driver standings as single message');
    console.log('Text length:', formattedText.length);

    // Send as a single message with the formatted text
    Pebble.sendAppMessage({
        REQUEST_TYPE: REQUEST_TYPES.GET_DRIVER_STANDINGS,
        DATA_TITLE: formattedText
    }, function () {
        console.log('Sent driver standings text successfully');
    }, function (e) {
        console.error('Failed to send driver standings:', e);
        console.error('Error details:', JSON.stringify(e));
    });
}

// Process standings data and send team standings to watch
function sendTeamStandingsToWatch(standingsData) {
    if (!standingsData || !standingsData.data) {
        console.error('Invalid standings data');
        return;
    }

    const constructorStandings = standingsData.data.constructorStandings;
    const constructors = standingsData.data.constructors;

    if (!constructorStandings || !constructors) {
        console.error('Missing constructor standings or constructors data');
        return;
    }

    // Convert constructorStandings object to array and sort by position
    const standingsArray = Object.values(constructorStandings).sort((a, b) => a.position - b.position);

    console.log(`Formatting ${standingsArray.length} team standings as text`);

    // Build a formatted string with all team standings
    const lines = [];
    standingsArray.forEach((standing) => {
        const constructor = constructors[standing.constructorId];
        if (!constructor) {
            console.error('Constructor not found:', standing.constructorId);
            return;
        }

        // Format: "1. Team Name - 123 pts"
        lines.push(`${standing.position}|${constructor.name}|${standing.points} pts`);
    });

    const formattedText = lines.join('\n');

    console.log('Sending team standings as single message');
    console.log('Text length:', formattedText.length);

    // Send as a single message with the formatted text
    Pebble.sendAppMessage({
        REQUEST_TYPE: REQUEST_TYPES.GET_TEAM_STANDINGS,
        DATA_TITLE: formattedText
    }, function () {
        console.log('Sent team standings text successfully');
    }, function (e) {
        console.error('Failed to send team standings:', e);
        console.error('Error details:', JSON.stringify(e));
    });
}

// Push a single timeline pin to the Rebble timeline API
function pushPin(token, pin) {
    return new Promise((resolve, reject) => {
        var xhr = new XMLHttpRequest();
        xhr.open('PUT', 'https://timeline-api.rebble.io/v1/user/pins/' + pin.id, true);
        xhr.setRequestHeader('Content-Type', 'application/json');
        xhr.setRequestHeader('X-User-Token', token);
        xhr.onload = function() {
            if (xhr.status === 200 || xhr.status === 201) {
                resolve();
            } else {
                reject(new Error('Timeline API error: ' + xhr.status));
            }
        };
        xhr.onerror = function() {
            reject(new Error('Network error pushing pin'));
        };
        xhr.send(JSON.stringify(pin));
    });
}

// Push timeline pins for all upcoming race events in the current season
function pushTimelinePins() {
    var season = getCurrentSeason();
    console.log('Pushing timeline pins for season:', season);

    Pebble.getTimelineToken(function(token) {
        fetchOverview(season).then(function(overviewData) {
            if (!overviewData || !overviewData.data) {
                console.error('Invalid overview data for timeline pins');
                return;
            }

            var now = new Date();
            var allRaces = Object.values(overviewData.data);

            // Only consider races that haven't fully passed
            var upcomingRaces = allRaces.filter(function(race) {
                return new Date(race.date) >= now;
            });

            var pins = [];
            upcomingRaces.forEach(function(race) {
                var events = race.schedule || [];
                events.forEach(function(event) {
                    var dateTimeStr = event.date + 'T' + event.time;
                    var eventTime = new Date(dateTimeStr);

                    // Skip events that have already passed
                    if (eventTime < now) return;

                    var abbrev = abbreviateEvent(event.label);
                    var location = race.circuit.city + ', ' + race.circuit.country;
                    pins.push({
                        id: 'f1-flashback-' + season + '-r' + race.round + '-' + abbrev.toLowerCase(),
                        time: eventTime.toISOString(),
                        layout: {
                            type: 'genericPin',
                            title:  event.label,
                            tinyIcon: 'system://images/TIMELINE_CALENDAR',
                            body: race.name + ' \u2022 ' + location
                        }
                    });
                });
            });

            // Push pins sequentially to avoid overwhelming the API
            pins.reduce(function(chain, pin) {
                return chain.then(function() {
                    return pushPin(token, pin).then(function() {
                        console.log('Pushed pin:', pin.id);
                    });
                });
            }, Promise.resolve()).then(function() {
                console.log('All timeline pins pushed successfully');
            }).catch(function(err) {
                console.error('Error pushing timeline pins:', err);
            });

        }).catch(function(err) {
            console.error('Failed to fetch overview for timeline pins:', err);
        });
    }, function(err) {
        console.error('Failed to get timeline token:', err);
    });
}

// Current season helper
function getCurrentSeason() {
    return new Date().getFullYear();
}

// Listen for messages from watch
Pebble.addEventListener('appmessage', function (e) {
    console.log('Received message from watch');

    // The payload uses string keys, not numeric keys
    const payload = e.payload;
    const requestType = payload.REQUEST_TYPE;
    const season = getCurrentSeason();

    console.log('Request type:', requestType);

    switch (requestType) {
        case REQUEST_TYPES.GET_OVERVIEW:
            console.log('Request: GET_OVERVIEW');
            fetchOverview(season)
                .then(data => sendRacesToWatch(data))
                .catch(error => console.error('Failed to get overview:', error));
            break;

        case REQUEST_TYPES.GET_RACE_DETAILS:
            console.log('Request: GET_RACE_DETAILS');
            const raceRound = payload.DATA_INDEX;
            console.log('Race round:', raceRound);
            fetchOverview(season)
                .then(data => sendRaceDetailsToWatch(data, raceRound))
                .catch(error => console.error('Failed to get race details:', error));
            break;

        case REQUEST_TYPES.GET_DRIVER_STANDINGS:
            console.log('Request: GET_DRIVER_STANDINGS');
            fetchStandings(season)
                .then(data => sendDriverStandingsToWatch(data))
                .catch(error => console.error('Failed to get driver standings:', error));
            break;

        case REQUEST_TYPES.GET_TEAM_STANDINGS:
            console.log('Request: GET_TEAM_STANDINGS');
            fetchStandings(season)
                .then(data => sendTeamStandingsToWatch(data))
                .catch(error => console.error('Failed to get team standings:', error));
            break;

        default:
            console.log('Unknown request type:', requestType);
    }
});

// App lifecycle
Pebble.addEventListener('ready', function () {
    console.log('PebbleKit JS ready!');
    console.log('Current season:', getCurrentSeason());

    // Push timeline pins only if enabled in settings
    var settings = JSON.parse(localStorage.getItem('clay-settings')) || {};
    var timelinePinsEnabled = settings.TimelinePins || false;
    if (timelinePinsEnabled) {
        try {
            pushTimelinePins();
        } catch (err) {
            console.error('Failed to push timeline pins:', err);
        }
    }
});

console.log('F1 Flashback JS loaded');
