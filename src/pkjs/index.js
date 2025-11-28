// PebbleKit JS - F1 Flashback Data Layer
// Handles API fetching, caching, and communication with watch

const CACHE_DURATION = 1000 * 60 * 60 * 24; // 24 hours
const BASE_URL = 'https://flashback.pages.dev';

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

    // Send upcoming races first, then past races
    const races = [...upcomingRaces, ...pastRaces];

    console.log(`Sending ${races.length} races to watch (${upcomingRaces.length} upcoming, ${pastRaces.length} past)`);

    // Send count first
    Pebble.sendAppMessage({
        REQUEST_TYPE: REQUEST_TYPES.GET_OVERVIEW,
        DATA_COUNT: races.length
    }, function () {
        console.log('Sent race count');
    }, function (e) {
        console.error('Failed to send race count:', e);
    });

    // Send each race
    races.forEach((race, index) => {
        const message = {
            REQUEST_TYPE: REQUEST_TYPES.GET_OVERVIEW,
            DATA_INDEX: index,
            DATA_TITLE: race.name,
            DATA_SUBTITLE: race.circuit.city + ', ' + race.circuit.country,
            DATA_EXTRA: race.date, // Race date for sorting
            DATA_ROUND: race.round // Round number (1-24)
        };

        // Add delay between messages to avoid overwhelming the watch
        // Real devices need more time than emulator (250ms vs 100ms)
        setTimeout(() => {
            Pebble.sendAppMessage(message,
                function () {
                    console.log(`Sent race ${index}: ${race.name}`);
                },
                function (e) {
                    console.error(`Failed to send race ${index}:`, e);
                    console.error('Error details:', JSON.stringify(e));
                }
            );
        }, index * 175);
    });
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

    console.log(`Sending ${events.length} events for ${race.name} (round ${raceRound})`);

    // Send count first
    Pebble.sendAppMessage({
        REQUEST_TYPE: REQUEST_TYPES.GET_RACE_DETAILS,
        DATA_COUNT: events.length
    }, function () {
        console.log('Sent event count');
    }, function (e) {
        console.error('Failed to send event count:', e);
    });

    // Send each event
    events.forEach((event, index) => {
        // Combine date and time into ISO format
        const dateTimeStr = event.date + 'T' + event.time;

        const message = {
            REQUEST_TYPE: REQUEST_TYPES.GET_RACE_DETAILS,
            DATA_INDEX: index,
            DATA_TITLE: event.label,
            DATA_SUBTITLE: dateTimeStr, // Will be parsed and formatted on watch
            DATA_EXTRA: '' // Reserved for future use
        };

        // Add delay between messages for real device compatibility
        setTimeout(() => {
            Pebble.sendAppMessage(message,
                function () {
                    console.log(`Sent event ${index}: ${event.label}`);
                },
                function (e) {
                    console.error(`Failed to send event ${index}:`, e);
                    console.error('Error details:', JSON.stringify(e));
                }
            );
        }, index * 175);
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

    console.log(`Sending ${standingsArray.length} driver standings to watch`);

    // Send count first
    Pebble.sendAppMessage({
        REQUEST_TYPE: REQUEST_TYPES.GET_DRIVER_STANDINGS,
        DATA_COUNT: standingsArray.length
    }, function () {
        console.log('Sent driver standings count');
    }, function (e) {
        console.error('Failed to send driver standings count:', e);
    });

    // Send each driver standing
    standingsArray.forEach((standing, index) => {
        const driver = drivers[standing.driverId];
        if (!driver) {
            console.error('Driver not found:', standing.driverId);
            return;
        }

        const fullName = driver.firstName + ' ' + driver.lastName;
        const code = driver.code || standing.driverId.toUpperCase().substring(0, 3);

        const message = {
            REQUEST_TYPE: REQUEST_TYPES.GET_DRIVER_STANDINGS,
            DATA_INDEX: index,
            DATA_TITLE: fullName,
            DATA_SUBTITLE: code,
            DATA_POINTS: standing.points,
            DATA_POSITION: standing.position
        };

        // Add delay between messages for real device compatibility
        setTimeout(() => {
            Pebble.sendAppMessage(message,
                function () {
                    console.log(`Sent driver ${index}: ${fullName} (${code}) - ${standing.points} pts`);
                },
                function (e) {
                    console.error(`Failed to send driver ${index}:`, e);
                    console.error('Error details:', JSON.stringify(e));
                }
            );
        }, index * 175);
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

    console.log(`Sending ${standingsArray.length} team standings to watch`);

    // Send count first
    Pebble.sendAppMessage({
        REQUEST_TYPE: REQUEST_TYPES.GET_TEAM_STANDINGS,
        DATA_COUNT: standingsArray.length
    }, function () {
        console.log('Sent team standings count');
    }, function (e) {
        console.error('Failed to send team standings count:', e);
    });

    // Send each team standing
    standingsArray.forEach((standing, index) => {
        const constructor = constructors[standing.constructorId];
        if (!constructor) {
            console.error('Constructor not found:', standing.constructorId);
            return;
        }

        const message = {
            REQUEST_TYPE: REQUEST_TYPES.GET_TEAM_STANDINGS,
            DATA_INDEX: index,
            DATA_TITLE: constructor.name,
            DATA_POINTS: standing.points,
            DATA_POSITION: standing.position
        };

        // Add delay between messages for real device compatibility
        setTimeout(() => {
            Pebble.sendAppMessage(message,
                function () {
                    console.log(`Sent team ${index}: ${constructor.name} - ${standing.points} pts`);
                },
                function (e) {
                    console.error(`Failed to send team ${index}:`, e);
                    console.error('Error details:', JSON.stringify(e));
                }
            );
        }, index * 175);
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

    // Send a ready message to the watch to confirm JS is running
    // This helps debug connectivity issues on real devices
    setTimeout(function() {
        console.log('Notifying watch that JS is ready');
    }, 1000);
});

Pebble.addEventListener('showConfiguration', function () {
    console.log('Showing configuration (not implemented)');
});

Pebble.addEventListener('webviewclosed', function (e) {
    console.log('Configuration closed');
});

console.log('F1 Flashback JS loaded');
