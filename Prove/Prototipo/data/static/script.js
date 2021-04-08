window.onload = () => {
  getRunningConf();
  getSavedConf();
};

var runningConfJSON, savedConfJSON;

function getRunningConf() {
  makeRequest('GET', '/get/running-conf').then((response) => {
    runningConfJSON = JSON.parse(response);
    document.getElementById('json_input').innerHTML = JSON.stringify(runningConfJSON, null, 4);

    document.getElementById('setApiUrl').value = runningConfJSON.apiUrl;
    document.getElementById('setFilterJSON').value = runningConfJSON.filterJSON;
    document.getElementById('setPath').value = runningConfJSON.path;
    document.getElementById('setMinValue').value = runningConfJSON.min_value;
    document.getElementById('setMaxValue').value = runningConfJSON.max_value;
    document.getElementById('setMinPwm').value = runningConfJSON.min_pwm;
    document.getElementById('setMaxPwm').value = runningConfJSON.max_pwm;
    document.getElementById('setRequestInterval').value = runningConfJSON.request_interval_ms;
  });
}

function getSavedConf() {
  makeRequest('GET', '/get/saved-conf').then((response) => {
    savedConfJSON = JSON.parse(response);
    document.getElementById('saved-conf').innerHTML = JSON.stringify(savedConfJSON, null, 4);
  });
}

// From https://stackoverflow.com/a/30008115
function makeRequest(method, url) {
  return new Promise(function (resolve, reject) {
    var xhr = new XMLHttpRequest();
    xhr.open(method, url);
    xhr.onload = function () {
      if (this.status >= 200 && this.status < 300) {
        resolve(xhr.response);
      } else {
        reject({
          status: this.status,
          statusText: xhr.statusText,
        });
      }
    };
    xhr.onerror = function () {
      reject({
        status: this.status,
        statusText: xhr.statusText,
      });
    };
    xhr.send();
  });
}
