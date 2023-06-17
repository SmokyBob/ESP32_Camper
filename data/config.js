function updateConfigs() {
  var str = "2?";

  var elems = document.getElementsByClassName("params");
  for (let index = 0; index < elems.length; index++) {
    const element = elems[index];
    str += element.name + "=";
    switch (element.id) {
      case "VDiv_Calib":
        str += element.value + "|" + element.attributes["data-value"] + "&";
        break;
      case "bAutomation":
        str += (element.checked) ? "1" : "0";
        break;
      default:
        str += element.value + "&";
        break;
    }
  }
  str = str.substring(0, str.length - 1 - 1);//remove the last character

  console.log(str);

  Socket.send(str); //Send WS message for processing
}

function addParam(name, desc, enumerator, value) {
  var el = document.createElement("div");
  el.className = "line";
  var label = document.createElement("span");
  label.innerText = desc + ":";
  label.style.display = "table-cell";
  var elInput = document.createElement("input");
  if (name == "bAutomation") {
    elInput.type = "checkbox";
    elInput.checked = (value == 0) ? false : true;
  } else {
    elInput.type = "number";
    elInput.value = value;
  }
  elInput.id = name;
  elInput.className = "params";
  elInput.style.display = "table-cell";
  elInput.name = enumerator;

  elInput.attributes["data-value"] = value

  el.appendChild(label);
  el.appendChild(elInput);

  var paramDiv = document.getElementById('params');
  paramDiv.appendChild(el);
}