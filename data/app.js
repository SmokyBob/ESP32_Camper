
var Socket;
function init() {
  Socket = new WebSocket('ws://' + window.location.hostname + '/ws');
  var timeout = setTimeout(function () {
    timedOut = true;

    try {
      Socket.close();
    } catch (error) {
      console.error(error);
      prependToLog(error);
    }

    timedOut = false;
  }, 5000);

  Socket.onmessage = function (event) {
    clearTimeout(timeout);
    processCommand(event);
  };
  Socket.onclose = function (e) {
    clearTimeout(timeout);
    console.log('Socket is closed. Reconnect will be attempted in 1 second.', e.reason);
    prependToLog('Socket is closed. Reconnect will be attempted in 1 second. ' + e.reason);
    setTimeout(function () {
      init();
    }, 1000);
  };

  Socket.onerror = function (err) {
    console.error('Socket encountered error: ', err.message, 'Closing socket');
    prependToLog('Socket encountered error: ' + err.message + ' Closing socket');
    try {
      Socket.close();
    } catch (error) {
      console.error(error);
      prependToLog(error);
    }
  };
}

function processCommand(event) {
  var obj = JSON.parse(event.data);

  prependToLog("millis: " + obj.millis + ", temperature: " + obj.temperature + " ,humidity: " + obj.humidity + " ");
}

function prependToLog(message) {
  var el = document.createElement("div");
  el.className = "line";
  el.innerText = message;

  var log = document.getElementById('log');
  log.insertBefore(el, log.firstChild);
}

window.onload = function (event) {
  init();
}