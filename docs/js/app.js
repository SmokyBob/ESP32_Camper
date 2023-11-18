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
dataChars["VOLTAGE"] = { uuid: "5b4c2c35-8a17-4d41-aec2-04a7dc1eaf91", id: "voltage", char: null };
dataChars["EXT_TEMPERATURE"] = { uuid: "226115b6-f631-4f82-b58d-b84487b55a64", id: "ext_temperature", char: null };
dataChars["EXT_HUMIDITY"] = { uuid: "b95cdb8a-7ee4-48c6-a818-fd11e60881f4", id: "ext_humidity", char: null };


//Command characteristics
var commandChar = {};
commandChar["WINDOW"] = { uuid: "4efa5b56-0426-42d7-857e-3ae3370b4a1d", id: "window", char: null };
commandChar["RELAY1"] = { uuid: "e8db3027-e095-435d-929c-f471669209c3", id: "relay1", char: null };
commandChar["RELAY2"] = { uuid: "4d15f090-6175-4e3c-b076-6ae0f69b7117", id: "relay2", char: null };
commandChar["AUTOMATION"] = { uuid: "ea7614e2-7eb9-4e1c-8ac4-5e64c3994264", id: "automation", char: null };
commandChar["DATETIME"] = { uuid: "2cdc00e8-907c-4f63-a284-2be098f8ea52", id: "datetime", char: null };

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
      for (let index = 0; index < characteristics.length; index++) {
        const element = characteristics[index];
        var characteristicFound = false;
        //datachars
        for (const [key, value] of Object.entries(dataChars)) {
          // console.log(`${key}: ${value.uuid}`);
          const cfg = value;
          if (element.uuid == cfg.uuid) {
            characteristicFound = true
            cfg.char = element;
            console.log("Data Characteristic discovered:", cfg.char.uuid);
            setTimeout(function () {
              cfg.char.addEventListener('characteristicvaluechanged', handleCharacteristicChange);
              cfg.char.startNotifications();
              console.log("Notifications Started.");
            }, (index) * 500);
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
              cfg.char = element;
              console.log("Data Characteristic discovered:", cfg.char.uuid);
              setTimeout(function () {
                cfg.char.addEventListener('characteristicvaluechanged', handleCharacteristicChange);
                cfg.char.startNotifications();
                console.log("Notifications Started.");
              }, (index) * 500);
              break;
            }
          }
        }
        if (characteristicFound == false) {
          console.log("UNKNOWN Characteristic discovered:", element.uuid);
        }
      }
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
        newValueReceived = event.target.value.getFloat32();
        document.getElementById(cfg.id).innerHTML = newValueReceived;
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
        if (cfg.id == "datetime") {
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

    location.href = location.href;//refresh the page to reinit the bt object
  } else {
    // Throw an error if Bluetooth is not connected
    console.error("Bluetooth is not connected.");
    window.alert("Bluetooth is not connected.")
  }
}

function getDateTime() {
  var myDate = new Date(new Date().getTime() + (-1 * (new Date().getTimezoneOffset()) * 60 * 1000));

  return myDate.toISOString();
}