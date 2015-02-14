var isReady = false;
var isFetching = false;
var callbacks = [];
var curTime;

var colour = {
  wob: 1,
  bow: 0
};
var bluetooth = {
  both: 2,
  one:  1,
  off:  0
};
var weather = {
  off:   0,
  on_15: 15,
  on_30: 30,
  on_60: 59
};

var imageId = {
  //  "012345678901"
  0:  "tornado",      //tornado
  1:  "trpcl storm",  //tropical storm
  2:  "hurricane",    //hurricane
  3:  "severe strms", //severe thunderstorms
  4:  "storms",       //thunderstorms
  5:  "hail",         //mixed rain and snow
  6:  "hail",         //mixed rain and sleet
  7:  "hail",         //mixed snow and sleet
  8:  "cold drizzle", //freezing drizzle
  9:  "drizzle",      //drizzle
  10: "cold rain",    //freezing rain
  11: "showers",      //showers
  12: "showers",      //showers
  13: "snow",         //snow flurries
  14: "light snow",   //light snow showers
  15: "windy snow",   //blowing snow
  16: "snow",         //snow
  17: "hail",         //hail
  18: "sleet",        //sleet
  19: "dust",         //dust
  20: "foggy",        //foggy
  21: "haze",         //haze
  22: "smoky",        //smoky
  23: "blustery",     //blustery
  24: "windy",        //windy
  25: "cold",         //cold
  26: "cloudy",       //cloudy
  27: "mstly cloudy", //mostly cloudy (night)
  28: "mstly cloudy", //mostly cloudy (day)
  29: "prtly cloudy", //partly cloudy (night)
  30: "prtly cloudy", //partly cloudy (day)
  31: "clear",        //clear (night)
  32: "clear",        //sunny
  33: "clear",        //fair (night)
  34: "cloudy",       //fair (day)
  35: "hail",         //mixed rain and hail
  36: "clear",        //hot
  37: "isltd storms", //isolated thunderstorms
  38: "sctrd storms", //scattered thunderstorms
  39: "sctrd storms", //scattered thunderstorms
  40: "sctrd shwrs",  //scattered showers
  41: "heavy snow",   //heavy snow
  42: "sctrd snow",   //scattered snow showers
  43: "heavy snow",   //heavy snow
  44: "prtly cloud",  //partly cloudy
  45: "storm",        //thundershowers
  46: "snow showers", //snow showers
  47: "isltd storms", //isolated thundershowers
  3200: "no data",    //not available
};


var locationOptions = { "timeout": 150000, "maximumAge": 600000 };

function weatherConfig() {
  var lopts = JSON.parse(localStorage.getItem("options"));
  return lopts.weather;
}
function getLocalWeather() {
  var lopts = JSON.parse(localStorage.getItem("options"));
  var icon;
  if (lopts.temp_unit == "c") {
    icon = localStorage.getItem("icon") + " " + localStorage.getItem("temperatureC");
  }
  else {
    icon = localStorage.getItem("icon") + " " + localStorage.getItem("temperatureF");
  }
  return icon;
}

function fetchWeather(latitude, longitude) {
  curTime = Math.floor((new Date).getTime()/1000);
  var lastFetch = localStorage.getItem("lastFetch");

  if (isFetching) {
    console.log("fetchWeather: already fetching, quit");
    return;
  }
  else if (curTime - lastFetch < 900) {
    var icon = getLocalWeather();
    transmitConfiguration({ "icon":icon });
    console.log("fetchWeather: already fetched recently, quit"+icon);
    return;
  }
  else {
    isFetching = true;
    console.log("fetchWeather: fetching now");

  var response;
  var woeid = -1;
  var query = encodeURI("select woeid from geo.placefinder where text=\""+latitude+","+longitude + "\" and gflags=\"R\"");
  var url = "http://query.yahooapis.com/v1/public/yql?q=" + query + "&format=json";
  var req = new XMLHttpRequest();
  req.open('GET', url, true);
  req.onload = function(e) {
    if (req.readyState == 4) {
      if (req.status == 200) {
        response = JSON.parse(req.responseText);
        if (response) {
          woeid = response.query.results.Result.woeid;
          getWeatherFromWoeid(woeid);
        }
      } else {
        console.log("Error");
      }
    }
  };
  req.send(null);
  }
}

function getWeatherFromWoeid(woeid) {
  var celsius = 'celsius';
  var query = encodeURI("select item.condition from weather.forecast where woeid = " + woeid +
                        " and u = " + (celsius ? "\"c\"" : "\"f\""));
  var url = "http://query.yahooapis.com/v1/public/yql?q=" + query + "&format=json";

  var response;
  var req = new XMLHttpRequest();
  req.open('GET', url, true);
  req.onload = function(e) {
    if (req.readyState == 4) {
      if (req.status == 200) {
        response = JSON.parse(req.responseText);
        if (response) {
          console.log(req.responseText);
          var condition = response.query.results.channel.item.condition;
          var temperatureC = condition.temp + "+C";
          var temperatureF = Math.round((condition.temp*9/5)+32) + "+F";

          var icon = imageId[condition.code];
          localStorage.setItem("lastFetch", curTime);
          localStorage.setItem("icon", icon);
          localStorage.setItem("temperatureC", temperatureC);
          localStorage.setItem("temperatureF", temperatureF);
          var icon2 = getLocalWeather();
          transmitConfiguration({ "icon":icon2 });
          console.log("fetchWeather: just fetched: "+icon2);
        }
      } else {
        console.log("Error");
      }
    }
  };
  req.send(null);
    isFetching = false;
    console.log("fetchWeather: finished fetching, quit");
}

function locationSuccess(pos) {
  var coordinates = pos.coords;
  var datetime = "======= lastsync: " + new Date();
  console.log(datetime);
  if(!isFetching && (weatherConfig()!="off"))fetchWeather(coordinates.latitude, coordinates.longitude);
  else console.log("locationSuccess: fetchWeather cancelled");
}

function locationError(err) {
  console.warn('location error (' + err.code + '): ' + err.message);
  transmitConfiguration({ "icon":"no location" });
}

function readyCallback(event) {
  isReady = true;
  var callback;
  while (callbacks.length > 0) {
    callback = callbacks.shift();
    callback(event);
  window.navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);
  }
}

// Retrieves stored configuration from localStorage.
function getOptions() {
  return localStorage.getItem("options") || ("{}");
}

// Stores options in localStorage.
function setOptions(options) {
  localStorage.setItem("options", options);
}

// Takes a string containing serialized JSON as input.  This is the
// format that is sent back from the configuration web UI.  Produces
// a JSON message to send to the watch face.
function prepareConfiguration(serialized_settings) {
  var settings = JSON.parse(serialized_settings);
  if(weatherConfig()) {
    var icon=getLocalWeather();
    return {
      "0": colour[settings.colour],
      "1": bluetooth[settings.bluetooth],
      "2": weather[settings.weather],
      "3": icon
    };
  }
  else {
    return {
      "0": colour[settings.colour],
      "1": bluetooth[settings.bluetooth],
      "2": weather[settings.weather]
    };
  }
}

// Takes a JSON message as input.  Sends the message to the watch.
function transmitConfiguration(settings) {
  //console.log('sending message: '+ JSON.stringify(settings));
  Pebble.sendAppMessage(settings, function(event) {
    // Message delivered successfully
  }, logError);
}

function logError(event) {
  console.log('Unable to deliver message with transactionId='+
              event.data.transactionId +' ; Error is'+ event.error.message);
}

function showConfiguration(event) {
    var opts = getOptions();
    var url  = "https://zecoj.github.io/dots-n-squares/";
    console.log(opts);
    Pebble.openURL(url + "#options=" + encodeURIComponent(opts));
}

function webviewclosed(event) {
  var resp = event.response;
  console.log('configuration response: '+ resp + ' ('+ typeof resp +')');

  var options = JSON.parse(resp);
  if (typeof options.colour === 'undefined' &&
      typeof options.bluetooth === 'undefined' &&
      typeof options.weather === 'undefined' &&
      typeof options.temp_unit === 'undefined') {
    return;
  }
  onReady(function() {
    setOptions(resp); //write to phone's local storage
    console.log('configuration response: '+ resp + ' ('+ typeof resp +')');
    var message = prepareConfiguration(resp); //prepare appmessage
    transmitConfiguration(message); //send appmessage
  });
}

function appmessage(event) {
  if(!isFetching && (weatherConfig()!="off"))window.navigator.geolocation.getCurrentPosition(locationSuccess, locationError, locationOptions);
  else console.log("appmessage: fetchWeather cancelled");
  console.log("message!");
}

function onReady(callback) {
  if (isReady) {
    callback();
  }
  else {
    callbacks.push(callback);
  }
}

Pebble.addEventListener("ready", readyCallback);
Pebble.addEventListener("showConfiguration", showConfiguration);
Pebble.addEventListener("webviewclosed", webviewclosed);
Pebble.addEventListener("appmessage", appmessage);

onReady(function(event) {
  var message = prepareConfiguration(getOptions());
  transmitConfiguration(message);
});