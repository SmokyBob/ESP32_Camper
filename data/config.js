function updateConfigs() {
  var str = "2?";

  var elems = document.getElementsByClassName("params");
  for (let index = 0; index < elems.length; index++) {
    const element = elems[index];
    str += element.name + "=";
    str += element.value + "&";
  }
  str = str.substring(0, str.length - 1 - 1);//remove the last character

  console.log(str);

  Socket.send(str); //Send WS message for processing
}

function addParam(name, enumerator, value) {
  var el = document.createElement("div");
  el.className = "line";
  var label = document.createElement("span");
  label.innerText = name;
  var elInput = document.createElement("input");
  elInput.type = "number";
  elInput.id = name;
  elInput.className = "params";
  elInput.name = enumerator;
  elInput.value = value;

  var paramDiv = document.getElementById('params');
  paramDiv.appendChild(el);
  paramDiv.appendChild(label);
  paramDiv.appendChild(elInput);
}