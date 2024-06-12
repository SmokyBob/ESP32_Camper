// DOM Elements
const connectButton = document.getElementById('connectBleButton');
const disconnectButton = document.getElementById('disconnectBleButton');
const bleStateContainer = document.getElementById('bleState');
const dataContainer = document.getElementById('data');

const timestampContainer = document.getElementById('timestamp');

//Define BLE Device Specs
var deviceName = 'ESP BLE';
var bleServiceUID = '41cc63d6-8918-4f8d-ab01-e95e4155ee41';

//Read only characteristics
var dataChars = {};
dataChars["VOLTS"] = { uuid: "5b4c2c35-8a17-4d41-aec2-04a7dc1eaf91", id: "VOLTS", char: null, notificationReg: false, lastUserNotification: null };
dataChars["EXT_TEMP"] = { uuid: "226115b6-f631-4f82-b58d-b84487b55a64", id: "EXT_TEMP", char: null, notificationReg: false, lastUserNotification: null };
dataChars["EXT_HUM"] = { uuid: "b95cdb8a-7ee4-48c6-a818-fd11e60881f4", id: "EXT_HUM", char: null, notificationReg: false, lastUserNotification: null };


//Command characteristics
var commandChar = {};
commandChar["B_WINDOW"] = { uuid: "4efa5b56-0426-42d7-857e-3ae3370b4a1d", id: "B_WINDOW", char: null, notificationReg: false };
commandChar["B_FAN"] = { uuid: "e8db3027-e095-435d-929c-f471669209c3", id: "B_FAN", char: null, notificationReg: false };
commandChar["B_HEATER"] = { uuid: "4d15f090-6175-4e3c-b076-6ae0f69b7117", id: "B_HEATER", char: null, notificationReg: false };
commandChar["B_AUTOMATION"] = { uuid: "ea7614e2-7eb9-4e1c-8ac4-5e64c3994264", id: "B_AUTOMATION", char: null, notificationReg: false };
commandChar["DATETIME"] = { uuid: "2cdc00e8-907c-4f63-a284-2be098f8ea52", id: "DATETIME", char: null, notificationReg: false };
commandChar["B_VOLT_LIM_IGN"] = { uuid: "70c74d81-5a61-43c0-b82b-08fcc9109ff4", id: "B_VOLT_LIM_IGN", char: null, notificationReg: false };

//Global Variables to Handle Bluetooth
var bleServer;
var bleService;

// Connect Button (search for BLE Devices only if BLE is available)
connectButton.addEventListener('click', (event) => {
  if (isWebBluetoothEnabled()) {
    connectToDevice();
  }
});

// Disconnect Button
disconnectButton.addEventListener('click', disconnectDevice);

// Check if BLE is available in your Browser
function isWebBluetoothEnabled() {
  if (!navigator.bluetooth) {
    console.log("Web Bluetooth API is not available in this browser!");
    bleStateContainer.innerHTML = "Web Bluetooth API is not available in this browser!";
    return false
  }
  console.log('Web Bluetooth API supported in this browser.');
  return true
}

function addCharNotifications() {
  const promArray = [];
  var i = 0;
  for (const [key, value] of Object.entries(dataChars)) {
    if (value.notificationReg == false) {
      i = i + 1;
      promArray.push(new Promise((resolve) => {
        setTimeout(() => {
          value.char.startNotifications()
            .then(() => {
              //Add Event listenener
              value.char.addEventListener('characteristicvaluechanged', handleCharacteristicChange);
              value.notificationReg = true;
              console.log(value.id + " notification added");
              resolve("ok");
            })
            .catch(error => {
              resolve("nope");
            });
        }, i * 100);
      })
      );
    }
  }
  for (const [key, value] of Object.entries(commandChar)) {
    if (value.notificationReg == false) {
      i = i + 1;
      promArray.push(new Promise((resolve) => {
        setTimeout(() => {
          value.char.startNotifications()
            .then(() => {
              //Add Event listenener
              value.char.addEventListener('characteristicvaluechanged', handleCharacteristicChange);
              value.notificationReg = true;
              console.log(value.id + " notification added");
              resolve("ok");
            })
            .catch(error => {
              resolve("nope");
            });
        }, i * 100);
      })
      );
    }
  }
  (async function () {
    const results = await Promise.all(promArray);
    // outputs `[2, 3, 5]` after five seconds
    console.log(results);

    //Check if some notificaton failes
    if (results.includes("nope")) {
      setTimeout(() => {
        addCharNotifications();
      }, 100);
    }
  })();
}

// Connect to BLE Device and Enable Notifications
function connectToDevice() {
  console.log('Initializing Bluetooth...');
  navigator.bluetooth.requestDevice({
    filters: [{ name: deviceName }],
    optionalServices: [bleServiceUID]
  })
    .then(device => {
      console.log('Device Selected:', device.name);
      bleStateContainer.innerHTML = 'Connected to device ' + device.name;
      bleStateContainer.style.color = "#24af37";
      dataContainer.style.display = "";
      device.addEventListener('gattservicedisconnected', onDisconnected);
      return device.gatt.connect();
    })
    .then(gattServer => {
      bleServer = gattServer;
      console.log("Connected to GATT Server");
      return bleServer.getPrimaryService(bleServiceUID);
    })
    .then(service => {
      bleService = service;
      console.log("Service discovered:", service.uuid);
      if (bleServer && bleServer.connected) {
        sendWriteChar("datetime", getDateTime());
      }
      return service.getCharacteristics();
    })
    .then(characteristics => {
      //Map characteristics
      for (const element of characteristics) {
        var characteristicFound = false;
        //datachars
        for (const [key, value] of Object.entries(dataChars)) {
          // console.log(`${key}: ${value.uuid}`);
          const cfg = value;
          if (element.uuid == cfg.uuid) {
            characteristicFound = true
            cfg.char = element;
            console.log("Data Characteristic discovered:", cfg.char.uuid);
            break;
          }
        }
        if (characteristicFound == false) {
          //commandChar
          for (const [key, value] of Object.entries(commandChar)) {
            // console.log(`${key}: ${value.uuid}`);
            const cfg = value;
            if (element.uuid == cfg.uuid) {
              characteristicFound = true
              cfg.char = element;
              console.log("Data Characteristic discovered:", cfg.char.uuid);
              break;
            }
          }
        }
        if (characteristicFound == false) {
          console.log("UNKNOWN Characteristic discovered:", element.uuid);
        }
      }

      //Add notitications recursively until all norifications are done
      addCharNotifications();

    })
    .catch(error => {
      console.log('Error: ', error);
    })
}

function onDisconnected(event) {
  console.log('Device Disconnected:', event.target.device.name);
  bleStateContainer.innerHTML = "Device disconnected";
  bleStateContainer.style.color = "#d13a30";
  dataContainer.style.display = "none";

  connectToDevice();
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

function handleCharacteristicChange(event) {
  var newValueReceived = new TextDecoder().decode(event.target.value);
  var characteristicFound = false;
  //datachars
  for (const [key, value] of Object.entries(dataChars)) {
    // console.log(`${key}: ${value.uuid}`);
    const cfg = value;
    if (event.srcElement.uuid == cfg.uuid) {
      characteristicFound = true;
      if (event.target.value.byteLength > 0) {
        document.getElementById(cfg.id).innerHTML = newValueReceived;
        if (cfg.id == "VOLTS") {
          document.getElementById(cfg.id).innerHTML = newValueReceived + " - " + getPercentage(newValueReceived) + "%";
        }
        if (Notification.permission === "granted") {
          //Check what to notify 
          if (cfg.id == "VOLTS") {
            var bNotify = false;
            if (cfg.lastUserNotification == null) {
              bNotify = true;
            } else {
              var diffMs = (new Date() - cfg.lastUserNotification); // milliseconds difference
              var diffMins = Math.round(((diffMs % 86400000) % 3600000) / 60000); // minutes

              if (diffMins >= 1) {
                bNotify = true;
              }
            }
            if (parseFloat(newValueReceived) <= 12.5 && bNotify) {
              //on android notifications can be created ONLY by Service worker
              //we send a postMessage API commant to the SW with the info for the notification
              navigator.serviceWorker.controller.postMessage({
                'type': 'notification',
                'title': 'Voltage low',
                'body': 'Current Voltage is ' + newValueReceived,
                'icon': 'img/bolt.svg'
              }
              );
              cfg.lastUserNotification = new Date();
            }
          }
          if (cfg.id == "EXT_TEMP") {
            var bNotify = false;
            if (cfg.lastUserNotification == null) {
              bNotify = true;
            } else {
              var diffMs = (new Date() - cfg.lastUserNotification); // milliseconds difference
              var diffMins = Math.round(((diffMs % 86400000) % 3600000) / 60000); // minutes

              if (diffMins >= 5) {
                bNotify = true;
              }
            }
            if (parseFloat(newValueReceived) <= 15.0 && bNotify) {
              //on android notifications can be created ONLY by Service worker
              //we send a postMessage API commant to the SW with the info for the notification
              navigator.serviceWorker.controller.postMessage({
                'type': 'notification',
                'title': 'Temperature low',
                'body': 'Current Temperature is ' + newValueReceived,
                'icon': 'img/thermometer.svg'
              }
              );

              cfg.lastUserNotification = new Date();
            }
          }

        }
      }
      break;
    }
  }
  if (characteristicFound == false) {
    //commandChar
    for (const [key, value] of Object.entries(commandChar)) {
      // console.log(`${key}: ${value.uuid}`);
      const cfg = value;
      if (event.srcElement.uuid == cfg.uuid) {
        characteristicFound = true;
        if (cfg.id == "DATETIME") {
          document.getElementById(cfg.id).innerHTML = newValueReceived;
        } else {
          var chk = document.getElementById(cfg.id);
          if (newValueReceived == "1") {
            chk.checked = true;
          } else {
            chk.checked = false;
          }
        }

        break;
      }
    }
  }

  if (characteristicFound == false) {
    console.log("Notification from UNKNOWN Characteristic:", event.srcElement.uuid);
  }

  console.log("Characteristic value changed: ", newValueReceived);
}

function sendWriteChar(propName, value) {
  var prop = commandChar[propName.toUpperCase()];
  if (bleServer && bleServer.connected) {
    bleService.getCharacteristic(prop.uuid)
      .then(characteristic => {
        console.log("Found the characteristic: ", characteristic.uuid);
        return characteristic.writeValue(new TextEncoder('utf-8').encode(value));
      })
      .then(() => {
        console.log("Value:", value);
      })
      .catch(error => {
        console.error("Error writing to the characteristic: ", error);
      });
  } else {
    console.error("Bluetooth is not connected. Cannot write to characteristic.")
    window.alert("Bluetooth is not connected. Cannot write to characteristic. \n Connect to BLE first!")
  }
}

function onCheckChanged(chkBox) {
  var val = '0';
  if (chkBox.checked) {
    val = '1';
  }
  sendWriteChar(chkBox.id, val);
}

function disconnectDevice() {
  console.log("Disconnect Device.");
  if (bleServer && bleServer.connected) {
    for (const [key, value] of Object.entries(dataChars)) {
      // console.log(`${key}: ${value.uuid}`);
      const cfg = value;
      cfg.char.stopNotifications()
        .then(() => {
          console.log("Notifications Stopped");
        })
        .catch(error => {
          console.log("An error occurred:", error);
        });
    }
    for (const [key, value] of Object.entries(commandChar)) {
      // console.log(`${key}: ${value.uuid}`);
      const cfg = value;
      cfg.char.stopNotifications()
        .then(() => {
          console.log("Notifications Stopped");
        })
        .catch(error => {
          console.log("An error occurred:", error);
        });
    }

    bleServer.disconnect();

    console.log("Device Disconnected");

  } else {
    // Throw an error if Bluetooth is not connected
    console.error("Bluetooth is not connected.");
    window.alert("Bluetooth is not connected.")
  }

  location.href = location.href;//refresh the page to reinit the bt object
}

function getDateTime() {
  var myDate = new Date(new Date().getTime() + (-1 * (new Date().getTimezoneOffset()) * 60 * 1000));

  return myDate.toISOString();
}


document.getElementById('show-notification-button').
  addEventListener('click', () => {
    if (document.getElementById('show-notification-button').className == 'enable') {
      Notification.requestPermission().then((permissionResult) => {
        if (permissionResult === 'granted') {
          document.getElementById('show-notification-button').innerText = 'Disable notifications';
          document.getElementById('show-notification-button').className = 'disable';
          navigator.serviceWorker.controller.postMessage({
            'type': 'enableNotification',
            'value': "true"
          }
          );
          localStorage.setItem('notification-permission', 'true');
        }
      })
    } else {
      document.getElementById('show-notification-button').innerText = 'Enable Device Notification';
      document.getElementById('show-notification-button').className = 'enable';
      navigator.serviceWorker.controller.postMessage({
        'type': 'enableNotification',
        'value': 'false'
      }
      );
      localStorage.setItem('notification-permission', 'false');
    }
  });
