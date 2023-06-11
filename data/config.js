function updateConfigs() {
  var str = "2?";

  var elems = document.getElementsByClassName("params");
  for (let index = 0; index < elems.length; index++) {
    const element = elems[index];
    str += element.name + "=";
    if (element.id == "VDiv_Calib") {
      str += element.value + "|" + element.attributes["data-value"] + "&";
    } else {
      str += element.value + "&";
    }
  }
  str = str.substring(0, str.length - 1 - 1);//remove the last character

  console.log(str);

  Socket.send(str); //Send WS message for processing
}

function addParam(name,desc, enumerator, value) {
  var el = document.createElement("div");
  el.className = "line";
  var label = document.createElement("span");
  label.innerText = desc + ":";
  label.style.display = "table-cell";
  var elInput = document.createElement("input");
  elInput.type = "number";
  elInput.id = name;
  elInput.className = "params";
  elInput.style.display = "table-cell";
  elInput.name = enumerator;
  elInput.value = value;
  elInput.attributes["data-value"] = value

  el.appendChild(label);
  el.appendChild(elInput);

  var paramDiv = document.getElementById('params');
  paramDiv.appendChild(el);
}