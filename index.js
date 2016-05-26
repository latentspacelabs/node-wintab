var wintab = require('./wintab');
setInterval(function () {
  var orientation = wintab.orientation();
  console.log(wintab.pressure(), 'azimuth: ', orientation.azimuth, 'altitude: ', orientation.altitude, 'twist: ', orientation.twist);
}, 1);