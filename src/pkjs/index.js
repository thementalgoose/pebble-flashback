// PebbleKit JS - F1 Flashback Data Layer
// Handles API fetching, caching, and communication with watch

const CACHE_DURATION = 1000 * 60 * 60 * 24; // 24 hours
const BASE_URL = 'https://sand.flashback.pages.dev';

// Import auto-generated message keys
var messageKeys = require('message_keys');

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

    // Convert data object to array and sort by round in reverse order
    // This ensures upcoming races are populated first
    const races = Object.values(overviewData.data).sort((a, b) => b.round - a.round);

    console.log(`Sending ${races.length} races to watch (reverse order)`);

    // Send count first
    Pebble.sendAppMessage({
        [messageKeys.REQUEST_TYPE]: REQUEST_TYPES.GET_OVERVIEW,
        [messageKeys.DATA_COUNT]: races.length
    }, function () {
        console.log('Sent race count');
    }, function (e) {
        console.error('Failed to send race count:', e);
    });

    // Send each race
    races.forEach((race, index) => {
        const message = {
            [messageKeys.REQUEST_TYPE]: REQUEST_TYPES.GET_OVERVIEW,
            [messageKeys.DATA_INDEX]: index,
            [messageKeys.DATA_TITLE]: race.name,
            [messageKeys.DATA_SUBTITLE]: race.circuit.city + ', ' + race.circuit.country,
            [messageKeys.DATA_EXTRA]: race.date, // Race date for sorting
            [messageKeys.DATA_ROUND]: race.round // Round number (1-24)
        };

        // Add small delay between messages to avoid overwhelming the watch
        setTimeout(() => {
            Pebble.sendAppMessage(message,
                function () {
                    console.log(`Sent race ${index}: ${race.name}`);
                },
                function (e) {
                    console.error(`Failed to send race ${index}:`, e);
                }
            );
        }, index * 100);
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
        [messageKeys.REQUEST_TYPE]: REQUEST_TYPES.GET_RACE_DETAILS,
        [messageKeys.DATA_COUNT]: events.length
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
            [messageKeys.REQUEST_TYPE]: REQUEST_TYPES.GET_RACE_DETAILS,
            [messageKeys.DATA_INDEX]: index,
            [messageKeys.DATA_TITLE]: event.label,
            [messageKeys.DATA_SUBTITLE]: dateTimeStr, // Will be parsed and formatted on watch
            [messageKeys.DATA_EXTRA]: '' // Reserved for future use
        };

        // Add small delay between messages
        setTimeout(() => {
            Pebble.sendAppMessage(message,
                function () {
                    console.log(`Sent event ${index}: ${event.label}`);
                },
                function (e) {
                    console.error(`Failed to send event ${index}:`, e);
                }
            );
        }, index * 100);
    });
}

// Current season helper
function getCurrentSeason() {
    return new Date().getFullYear();
}

// Listen for messages from watch
Pebble.addEventListener('appmessage', function (e) {
    console.log('Received message from watch');
    const payload = e.payload;
    const requestType = payload[messageKeys.REQUEST_TYPE];
    const season = getCurrentSeason();

    switch (requestType) {
        case REQUEST_TYPES.GET_OVERVIEW:
            console.log('Request: GET_OVERVIEW');
            fetchOverview(season)
                .then(data => sendRacesToWatch(data))
                .catch(error => console.error('Failed to get overview:', error));
            break;

        case REQUEST_TYPES.GET_RACE_DETAILS:
            console.log('Request: GET_RACE_DETAILS');
            const raceRound = payload[messageKeys.DATA_INDEX];
            console.log('Race round:', raceRound);
            fetchOverview(season)
                .then(data => sendRaceDetailsToWatch(data, raceRound))
                .catch(error => console.error('Failed to get race details:', error));
            break;

        case REQUEST_TYPES.GET_DRIVER_STANDINGS:
            console.log('Request: GET_DRIVER_STANDINGS (not implemented)');
            // TODO: Implement driver standings
            break;

        case REQUEST_TYPES.GET_TEAM_STANDINGS:
            console.log('Request: GET_TEAM_STANDINGS (not implemented)');
            // TODO: Implement team standings
            break;

        default:
            console.log('Unknown request type:', requestType);
    }
});

// App lifecycle
Pebble.addEventListener('ready', function () {
    console.log('PebbleKit JS ready!');
    console.log('Current season:', getCurrentSeason());
});

Pebble.addEventListener('showConfiguration', function () {
    console.log('Showing configuration (not implemented)');
});

Pebble.addEventListener('webviewclosed', function (e) {
    console.log('Configuration closed');
});

console.log('F1 Flashback JS loaded');
