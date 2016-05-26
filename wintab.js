var wintab = require('./build/Release/wintab');

setInterval(function () {
    wintab.peekMessage();
    if (wintab.checkOverlapped())
        wintab.enableContext();
}, 1);

function minLevel() {
    return wintab.minPressure();
}
exports.minLevel = minLevel;

function maxLevel() {
    return wintab.maxPressure();
}
exports.maxLevel = maxLevel;

function levels() {
    var min = wintab.minPressure();
    if (min === null)
        return null;
    var max = wintab.maxPressure();
    return max - min + 1;
}
exports.levels = levels;

function isEraser() {
    return (wintab.isEraser() | 0) == 1;
}
exports.isEraser = isEraser;

function pressure() {
    var _pressure = wintab.pressure();
    if (_pressure === null)
        return null;
    return (_pressure - wintab.minPressure()) / (levels() - 1);
}
exports.pressure = pressure;

function orientation() {
    return {
        azimuth: wintab.azimuth() / 10, // 0 - 3600 => 0 - 360
        altitude: wintab.altitude() / 10 /*wintab.altitudeMax()*/ // -900 - 900 => -90 - 90
    };
}
exports.orientation = orientation;

function allData() {
    var o = orientation();
    return {
        x: wintab.x(),
        y: wintab.y(),
        pressure: pressure(),
        azimuth: o.azimuth,
        altitude: o.altitude,
        isEraser: isEraser()
    };
}
exports.allData = allData;