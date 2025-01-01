
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
    sendWSCommand("DATETIME", myDate.toISOString());
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

var batt_perc_12_list = [
  { voltage: 13.60, percentage: 100 },
  { voltage: 13.40, percentage: 99 },
  { voltage: 13.32, percentage: 90 },
  { voltage: 13.28, percentage: 80 },
  { voltage: 13.20, percentage: 70 },
  { voltage: 13.16, percentage: 60 },
  { voltage: 13.13, percentage: 50 },
  { voltage: 13.10, percentage: 40 },
  { voltage: 13.00, percentage: 30 },
  { voltage: 12.90, percentage: 20 },
  { voltage: 12.80, percentage: 17 },
  { voltage: 12.50, percentage: 14 },
  { voltage: 12.00, percentage: 9 },
  { voltage: 10.00, percentage: 0 }];

function getPercentage(currVoltage) {
  var toRet = "";
  var last_Voltage = parseFloat(currVoltage);
  for (i = 0; i < batt_perc_12_list.length; i++) {
    /* table lookup */
    if (last_Voltage >= batt_perc_12_list[i].voltage) {
      toRet = "" + batt_perc_12_list[i].percentage;
      break;
    }
  }
  return toRet;
}

function processCommand(event) {
  var obj = JSON.parse(event.data);

  //ccess properties and update UI elements
  document.getElementById("DATETIME").innerHTML = obj["DATETIME"];
  document.getElementById("VOLTS").innerHTML = obj["VOLTS"] + " - " + getPercentage(obj["VOLTS"]) + "%";
  document.getElementById("CAR_VOLTS").innerHTML = obj["CAR_VOLTS"] + " - " + getPercentage(obj["CAR_VOLTS"]) + "%";
  document.getElementById("EXT_TEMP").innerHTML = obj["EXT_TEMP"];
  document.getElementById("EXT_HUM").innerHTML = obj["EXT_HUM"];

  var checked = obj["B_WINDOW"];
  if (checked == "1") {
    document.getElementById("B_WINDOW").checked = true;
  } else {
    document.getElementById("B_WINDOW").checked = false;
  }

  checked = obj["B_FAN"];
  document.getElementById("B_FAN").checked = (checked == "1");

  checked = obj["B_HEATER"];
  document.getElementById("B_HEATER").checked = (checked == "1");

  checked = obj["B_AUTOMATION"];
  document.getElementById("B_AUTOMATION").checked = (checked == "1");

  checked = obj["B_VOLT_LIM_IGN"];
  document.getElementById("B_VOLT_LIM_IGN").checked = (checked == "1");

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
propMap["B_WINDOW"] = { id: "6", type: "0" };
propMap["B_FAN"] = { id: "7", type: "0" };
propMap["RELAY2"] = { id: "8", type: "0" };
propMap["DATETIME"] = { id: "5", type: "0" };
propMap["B_AUTOMATION"] = { id: "12", type: "1" };
propMap["B_VOLT_LIM_IGN"] = { id: "9", type: "1" };

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