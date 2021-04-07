window.onload = () => {
  getRunningConf();
  getSavedConf();
};

var runningConfJSON, savedConfJSON;

function getRunningConf() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function () {
    if (this.readyState == 4 && this.status == 200) {
      runningConfJSON = JSON.parse(this.responseText);
      document.getElementById('json_input').innerHTML = JSON.stringify(runningConfJSON, null, 4);
    }
  };
  xhttp.open('GET', '/get/running-conf', true);
  xhttp.send();
}

function getSavedConf() {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function () {
    if (this.readyState == 4 && this.status == 200) {
      savedConfJSON = JSON.parse(this.responseText);
      document.getElementById('saved-conf').innerHTML = JSON.stringify(savedConfJSON, null, 4);
    }
  };
  xhttp.open('GET', '/get/saved-conf', true);
  xhttp.send();
}
