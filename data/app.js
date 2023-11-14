
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

  Socket.onopen = function (event) {
    //Send updated date from the device
    var myDate = new Date(new Date().getTime() + (-1 * (new Date().getTimezoneOffset()) * 60 * 1000));
    sendWSCommand("datetime", myDate.toISOString());
  }

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

  //ccess properties and update UI elements
  document.getElementById("datetime").innerHTML = obj["datetime"];
  document.getElementById("voltage").innerHTML = obj["voltage"];
  document.getElementById("ext_temperature").innerHTML = obj["ext_temperature"];
  document.getElementById("ext_humidity").innerHTML = obj["ext_humidity"];

  var checked = obj["window"];
  if (checked == "1") {
    document.getElementById("window").checked = true;
  } else {
    document.getElementById("window").checked = false;
  }

  checked = obj["relay1"];
  if (checked == "1") {
    document.getElementById("relay1").checked = true;
  } else {
    document.getElementById("relay1").checked = false;
  }

  checked = obj["relay2"];
  if (checked == "1") {
    document.getElementById("relay2").checked = true;
  } else {
    document.getElementById("relay2").checked = false;
  }

  checked = obj["automation"];
  if (checked == "1") {
    document.getElementById("automation").checked = true;
  } else {
    document.getElementById("automation").checked = false;
  }

  prependToLog(event.data);
}

function prependToLog(message) {
  var el = document.createElement("div");
  el.className = "line";
  el.innerText = message;

  var log = document.getElementById('log');
  if (log != null) {
    log.insertBefore(el, log.firstChild);
  }

}

var propMap = {};
propMap["WINDOW"] = { id: "5", type: "1" };
propMap["RELAY1"] = { id: "6", type: "1" };
propMap["RELAY2"] = { id: "7", type: "1" };
propMap["DATETIME"] = { id: "4", type: "1" };
propMap["AUTOMATION"] = { id: "18", type: "2" };

function sendWSCommand(propName, value) {

  var prop = propMap[propName.toUpperCase()];
  var str = prop.type + "?" + prop.id + "=" + value;
  console.log(str);
  Socket.send(str); //Send WS message for processing
}

function onCheckChanged(chkBox) {
  var val = '0';
  if (chkBox.checked) {
    val = '1';
  }
  sendWSCommand(chkBox.id, val);
}

window.onload = function (event) {
  init();


}