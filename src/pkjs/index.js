var F1_API_URL = 'https://flashback.pages.dev/overview/2025.json';

// Helper to format date
function formatDate(dateStr) {
  var date = new Date(dateStr);
  var months = ['Jan', 'Feb', 'Mar', 'Apr', 'May', 'Jun', 
                'Jul', 'Aug', 'Sep', 'Oct', 'Nov', 'Dec'];
  return months[date.getMonth()] + ' ' + date.getDate();
}

// Fetch F1 calendar data
function fetchF1Calendar() {
  console.log('Fetching F1 calendar...');
  
  var xhr = new XMLHttpRequest();
  xhr.open('GET', F1_API_URL, true);
  xhr.onload = function() {
    if (xhr.readyState === 4 && xhr.status === 200) {
      try {
        var response = JSON.parse(xhr.responseText);
        var data = response.data;
        var races = [];
        
        // Extract race data
        for (var key in data) {
          if (data.hasOwnProperty(key)) {
            var race = data[key];
            races.push({
              name: race.name,
              location: race.circuit.city,
              date: formatDate(race.date),
              round: race.round
            });
          }
        }
        
        // Sort by round
        races.sort(function(a, b) { return a.round - b.round; });
        
        console.log('Fetched ' + races.length + ' races');
        sendRacesToWatch(races);
        
      } catch (e) {
        console.error('Error parsing F1 data: ' + e.message);
      }
    } else {
      console.error('Request failed: ' + xhr.status);
    }
  };
  xhr.send();
}

// Send race data to watch
function sendRacesToWatch(races) {
  // First send the count using numeric key
  Pebble.sendAppMessage({
    0: races.length
  });
  
  // Then send each race using numeric keys
  races.forEach(function(race, index) {
    setTimeout(function() {
      Pebble.sendAppMessage({
        4: index,           // KEY_RACE_INDEX
        1: race.name,       // KEY_RACE_NAME
        2: race.location,   // KEY_RACE_LOCATION
        3: race.date        // KEY_RACE_DATE
      }, function() {
        console.log('Sent race ' + index + ': ' + race.name);
      }, function(e) {
        console.error('Failed to send race ' + index + ': ' + e.error.message);
      });
    }, index * 100); // Delay to avoid overwhelming the watch
  });
}

// Listen for when the watchapp is opened
Pebble.addEventListener('ready', function() {
  console.log('PebbleKit JS ready!');
  fetchF1Calendar();
});

// Listen for messages from watch
Pebble.addEventListener('appmessage', function(e) {
  console.log('Received message from watch, fetching data...');
  fetchF1Calendar();
});